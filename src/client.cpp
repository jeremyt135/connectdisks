#include "four-across/networking/client/client.hpp"

#include "four-across/networking/server/server.hpp"
#include "four-across/game/game.hpp"

#include "type-utility.hpp"
#include "logging.hpp"

#include <array>
#include <iostream>

using boost::asio::ip::address_v4;
using boost::asio::ip::tcp;

using typeutil::toScopedEnum;

namespace game
{
	namespace networking
	{
		namespace client
		{
			Client::Client(boost::asio::io_service & ioService)
				:
				socket{ioService},
				playerId{0},
				game{nullptr}
			{
			}

			Client::~Client()
			{
			}

			void Client::connectToServer(std::string address, uint16_t port)
			{
				try
				{
					socket.connect(tcp::endpoint{address_v4::from_string(address), port});
				}
				catch (std::exception& error)
				{
					printDebug("Client::connectToServer: error: ", error.what(), "\n");
					throw;
				}

				waitForMessages();
			}

			void Client::toggleReady()
			{
				// tell server we're ready to play
				auto message = new Message{};
				message->type = MessageType::ready;
				message->data[0] = playerId;
				sendMessage(message);
			}

			void Client::disconnect()
			{
				handleDisconnect();
			}

			void Client::waitForMessages()
			{
				printDebug("Client waiting for messages\n");
				// read message from server
				std::shared_ptr<Message> message{new Message{}};
				boost::asio::async_read(socket,
					boost::asio::buffer(message.get(), sizeof(Message)),
					std::bind(
						&Client::onReadSocket,
						this,
						message,
						std::placeholders::_1,
						std::placeholders::_2
					));
			}

			void Client::onConnect(const boost::system::error_code & error)
			{
				if (!error.failed())
				{
					printDebug("Client disconnected\n");
				}
				else
				{
					printDebug("Client::handleConnection: error: ", error.message(), "\n");
				}
			}

			void Client::onReadSocket(std::shared_ptr<Message> message, const boost::system::error_code & error, size_t len)
			{
				if (!error.failed())
				{
					printDebug("Client received message\n");

					if (len == 0)
					{
						return;
					}

					const auto response = message->type;

					switch (response)
					{
					case MessageType::connected:
						setPlayerId(message->data[0]);
						connected(playerId);
						break;
					case MessageType::gameStart:
					{
						const auto numPlayers = message->data[0];
						const auto numCols = message->data[1];
						const auto numRows = message->data[2];
						const auto first = message->data[3];
						printDebug(
							"Client is in a lobby that has started playing with "
							, static_cast<int>(numPlayers), " players\n");
						printDebug(
							"Board size is set to cols=", static_cast<int>(numCols), ", rows=",
							static_cast<int>(numRows), "\n");

						startGame(numPlayers, first, numCols, numRows);
					}
					break;
					case MessageType::gameEnd:
					{
						printDebug("Client is in a game that ended; returned to lobby\n");
						const auto winner = message->data[0];

						stopGame(winner);
					}
					break;
					case MessageType::takeTurn:
					{
						const auto id = message->data[0];
						const auto column = message->data[1];
						if (id == playerId && column == static_cast<uint8_t>(-1))
						{
							printDebug("Time for client to take turn\n");
							requestTurn();
						}
					}
					break;
					case MessageType::turnResult:
					{
						printDebug("Client received results of turn\n");
						const auto turnResult = toScopedEnum<FourAcross::TurnResult>::cast(message->data[0]);
						const auto column = message->data[1];
						checkTurnResult(turnResult, column);
					}
					break;
					case MessageType::update:
					{
						printDebug("Client received an opponent's turn update\n");
						const auto player = message->data[0];
						const auto column = message->data[1];
						takeOpponentTurn(player, column);
					}
					break;
					default:
						break;
					}

					waitForMessages();
				}
				else
				{
					switch (error.value())
					{
					case boost::asio::error::eof:
					case boost::asio::error::connection_aborted:
					case boost::asio::error::connection_reset:
						printDebug("Client::handleRead: connection ended \n");
						break;
					default:
						printDebug("Client::handleRead: unknown error occurred: ", error.message(), "\n");
						break;
					}

					handleDisconnect();
				}
			}

			void Client::onWriteSocket(std::shared_ptr<Message> message, const boost::system::error_code & error, size_t len)
			{
				if (!error.failed())
				{
					printDebug("Sent message to server\n");
				}
				else
				{
					printDebug("Client::handleWrite: error: ", error.message(), "\n");
				}
			}

			void Client::setPlayerId(uint8_t id)
			{
				playerId = id;
				printDebug("Client id set to: ", static_cast<int>(playerId), "\n");
			}

			void Client::handleDisconnect()
			{
				//closeSocket();
				disconnected(); // tell lobby we're disconnecting
			}

			void Client::sendMessage(Message* message)
			{
				auto messagePtr = std::shared_ptr<Message>(message);
				boost::asio::async_write(socket,
					boost::asio::buffer(messagePtr.get(), sizeof(Message)),
					std::bind(
						&Client::onWriteSocket,
						this,
						messagePtr,
						std::placeholders::_1,
						std::placeholders::_2
					));
			}

			void Client::startGame(uint8_t numPlayers, uint8_t first, uint8_t cols, uint8_t rows)
			{
				game.reset(new FourAcross{numPlayers, first, cols, rows});
				gameStarted(numPlayers, first, cols, rows);
			}

			void Client::stopGame(uint8_t winner)
			{
				gameEnded(winner);
			}

			void Client::checkTurnResult(FourAcross::TurnResult result, uint8_t column)
			{
				switch (result)
				{
				case FourAcross::TurnResult::success:
					game->takeTurn(playerId, column);
					turnResult(result, column);
					break;
				case FourAcross::TurnResult::error:
					printDebug("Client::onTurnResult: error taking turn, not requesting a new turn unless server asks\n");
					break;
				default:
					printDebug("Client::onTurnResult: Unsuccessful turn, client sending new turn\n");
					turnResult(result, column);
					requestTurn(); // tell user to take turn again
					break;
				}

			}

			void Client::takeOpponentTurn(uint8_t player, uint8_t col)
			{
				// assume all turns sent to us from server are good
				game->takeTurn(player, col);

				gameUpdated(player, col);
			}

			const FourAcross * Client::getGame() const noexcept
			{
				return game.get();
			}

			void Client::takeTurn(uint8_t column)
			{
				// send turn to server
				auto message = new Message{};
				message->type = MessageType::takeTurn;
				message->data[0] = playerId; // send our id with the turn
				message->data[1] = column; // send column
				sendMessage(message);
			}

		}
	}
}