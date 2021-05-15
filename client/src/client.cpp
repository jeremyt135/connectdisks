#include "four-across/networking/client/client.hpp"

#include "four-across/networking/server/server.hpp"
#include "four-across/game/game.hpp"

#include "type-utility.hpp"
#include "logging.hpp"

#include <boost/endian/conversion.hpp>

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
			Client::Client()
				:
				playerId{0},
				isConnected{false},
				game{nullptr}
			{

				socket.reset(new tcp::socket{ioContext});
			}

			Client::~Client()
			{
				stopContext();
			}

			void Client::connect(std::string address, uint16_t port)
			{
				try
				{
					socket->connect(tcp::endpoint{address_v4::from_string(address), port});
				}
				catch (std::exception& error)
				{
					printDebug("Client::connectToServer: error: ", error.what(), "\n");
					throw;
				}
				waitForMessages();
				ioContext.run();
			}

			void Client::disconnect()
			{
				socket->close();
				socket.reset();
			}

			void Client::stop()
			{
				disconnect();
				stopContext();
			}

			void Client::toggleReady()
			{
				// tell server we're ready to play
				auto message = new Message{};
				message->type = MessageType::ready;
				message->data[0] = playerId;
				sendMessage(message);
			}

			void Client::waitForMessages()
			{
				printDebug("Client waiting for messages\n");
				// read message from server
				std::shared_ptr<Message> message{new Message{}};
				if (socket != nullptr)
				{
					boost::asio::async_read(*socket,
						boost::asio::buffer(message.get(), sizeof(Message)),
						std::bind(
							&Client::handleRead,
							this,
							message,
							std::placeholders::_1,
							std::placeholders::_2
						));
				}
			}

			void Client::handleConnect(const boost::system::error_code & error)
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

			void Client::handleRead(std::shared_ptr<Message> message, const boost::system::error_code & error, size_t len)
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
						isConnected = true;
						onConnect(playerId);
						break;
					case MessageType::inQueue:
					{
						uint64_t position{0};
						memcpy(&position, &message->data[0], sizeof(uint64_t));
						boost::endian::big_to_native_inplace(position);
						onQueueUpdate(position);
					}
					break;
					case MessageType::ping:
					{
						//printDebug("PONG\n");
						sendPong();
					}
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
							handleTurnRequest();
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

					onDisconnect();
				}
			}

			void Client::handleWrite(std::shared_ptr<Message> message, const boost::system::error_code & error, size_t len)
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

			void Client::onConnect(uint8_t playerId)
			{
				connected(playerId);
			}

			void Client::onDisconnect()
			{
				//closeSocket();
				isConnected = false;
				disconnected(); // tell lobby we're disconnecting
			}

			void Client::onQueueUpdate(uint64_t queuePosition)
			{
				queueUpdated(queuePosition);
			}

			void Client::sendMessage(Message* message)
			{
				auto messagePtr = std::shared_ptr<Message>(message);
				if (socket != nullptr)
				{
					boost::asio::async_write(*socket,
						boost::asio::buffer(messagePtr.get(), sizeof(Message)),
						std::bind(
							&Client::handleWrite,
							this,
							messagePtr,
							std::placeholders::_1,
							std::placeholders::_2
						));
				}
			}

			void Client::sendPong()
			{
				auto message = new Message{};
				message->type = MessageType::pong;
				sendMessage(message);
			}

			void Client::startGame(uint8_t numPlayers, uint8_t first, uint8_t cols, uint8_t rows)
			{
				game.reset(new FourAcross{numPlayers, first, cols, rows});
				onGameStart(numPlayers, first, cols, rows);
			}

			void Client::stopGame(uint8_t winner)
			{
				onGameEnd(winner);
			}

			void Client::checkTurnResult(FourAcross::TurnResult result, uint8_t column)
			{
				switch (result)
				{
				case FourAcross::TurnResult::success:
					game->takeTurn(playerId, column);
					onTurnResult(result, column);
					break;
				case FourAcross::TurnResult::error:
					printDebug("Client::onTurnResult: error taking turn, not requesting a new turn unless server asks\n");
					break;
				default:
					printDebug("Client::onTurnResult: Unsuccessful turn, client sending new turn\n");
					onTurnResult(result, column);
					handleTurnRequest();
					break;
				}

			}

			void Client::takeOpponentTurn(uint8_t player, uint8_t col)
			{
				// assume all turns sent to us from server are good
				game->takeTurn(player, col);

				onGameUpdate(player, col);
			}

			const FourAcross * Client::getGame() const noexcept
			{
				return game.get();
			}

			void Client::stopContext()
			{
				ioContext.stop();
			}

			void Client::sendTurn(uint8_t column)
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