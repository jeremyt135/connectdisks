#include "four-across/networking/server/connection.hpp"
#include "four-across/networking/server/gamelobby.hpp"

#include "type-utility.hpp"
#include "logging.hpp"

#include <algorithm>
#include <iostream>
#include <functional>

using boost::asio::ip::address_v4;
using boost::asio::ip::tcp;

using typeutil::toUnderlyingType;

namespace game
{
	namespace networking
	{
		namespace server
		{

			std::shared_ptr<Connection> Connection::create(boost::asio::io_service & ioService, GameLobby* lobby)
			{
				return std::shared_ptr<Connection>{new Connection{ioService, lobby}};
			}

			void Connection::onGameStart()
			{
				// send id to client
				std::shared_ptr<Message> message{new Message{}};
				message->type = MessageType::gameStart;

				// send the number of players
				message->data[0] = lobby->getNumPlayers();

				// send the board dimensions
				auto* game = lobby->getGame();

				message->data[1] = game->getNumColumns();
				message->data[2] = game->getNumRows();
				message->data[3] = game->getCurrentPlayer();

				sendMessage(message);
			}

			void Connection::onGameEnd(uint8_t player)
			{
				sendWinner(player);
			}

			void Connection::sendWinner(uint8_t winner)
			{
				// tell client game has ended and which player won
				std::shared_ptr<Message> message{new Message{}};
				message->type = MessageType::gameEnd;
				message->data[0] = winner;
				sendMessage(message);
			}

			void Connection::waitForMessages()
			{
				printDebug("Connection waiting to read message\n");
				// read a message from the client, handle in handleRead
				std::shared_ptr<Message> message{new Message{}};
				boost::asio::async_read(socket,
					boost::asio::buffer(message.get(), sizeof(Message)),
					std::bind(
						&Connection::handleRead,
						this,
						message,
						std::placeholders::_1,
						std::placeholders::_2
					));
			}

			void Connection::setId(uint8_t id)
			{
				if (this->id == 0)
				{
					this->id = id;

					// send id to client
					std::shared_ptr<Message> message{new Message{}};
					message->type = MessageType::connected;
					message->data[0] = id;
					sendMessage(message);
				}
			}

			uint8_t Connection::getId() const noexcept
			{
				return id;
			}

			void Connection::setGameLobby(GameLobby * lobby)
			{
				using std::placeholders::_1;
				using std::placeholders::_2;
				using std::placeholders::_3;
				using std::bind;

				this->lobby = lobby;
				lobby->addTurnHandler(GameLobby::TurnHandler(bind(&Connection::onTurn, this, _1)).track_foreign(shared_from_this()));
				lobby->addTurnResultHandler(GameLobby::TurnResultHandler(bind(&Connection::onUpdate, this, _1, _2, _3)).track_foreign(shared_from_this()));
				lobby->addGameStartHandler(GameLobby::GameStartHandler(bind(&Connection::onGameStart, this)).track_foreign(shared_from_this()));
				lobby->addGameEndHandler(GameLobby::GameEndHandler(bind(&Connection::onGameEnd, this, _1)).track_foreign(shared_from_this()));
			}

			boost::asio::ip::tcp::socket& Connection::getSocket()
			{
				return socket;
			}

			Connection::Connection(boost::asio::io_service & ioService, GameLobby* lobby) :
				lobby{lobby},
				socket{ioService},
				id{0}
			{
			}

			void Connection::onTurn(uint8_t playerId)
			{
				if (playerId == id)
				{
					// tell client it's their turn
					std::shared_ptr<Message> message{new Message{}};
					message->type = MessageType::takeTurn;
					message->data[0] = playerId;
					message->data[1] = static_cast<uint8_t>(-1); // requesting turn
					sendMessage(message);
				}
			}


			void Connection::onUpdate(uint8_t playerId, uint8_t col, FourAcross::TurnResult result)
			{

				if (playerId != id)
				{
					if (result == FourAcross::TurnResult::success)
					{
						// tell client that a successful turn was taken by another player
						std::shared_ptr<Message> message{new Message{}};
						message->type = MessageType::update;
						message->data[0] = playerId;
						message->data[1] = col;
						sendMessage(message);
					}
				}
				else
				{
					handleTurnResult(result, col);
				}
			}

			void Connection::sendMessage(std::shared_ptr<Message> message)
			{
				boost::asio::async_write(socket,
					boost::asio::buffer(message.get(), sizeof(Message)),
					std::bind(
						&Connection::handleWrite,
						this,
						message,
						std::placeholders::_1,
						std::placeholders::_2
					));
			}

			void Connection::handleRead(std::shared_ptr<Message> message, const boost::system::error_code & error, size_t len)
			{
				printDebug("Connection trying to read message\n");
				if (!error.failed())
				{

					if (len == 0)
					{
						printDebug("Connection received 0 length message\n");
						return;
					}

					const auto response = message->type;
					switch (response)
					{
					case MessageType::ready:
					{
						const auto id = message->data[0];
						if (id == this->id)
						{
							handleClientReady();
						}
					}
					break;
					case MessageType::takeTurn:
					{
						// received turn
						const auto id = message->data[0];
						const auto column = message->data[1];
						if (id == this->id && column != static_cast<uint8_t>(-1))
						{
							printDebug("Connection: client wants to move in column: ", static_cast<int>(column), "\n");
							tookTurn(shared_from_this(), column);
						}
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
						handleDisconnect();
						break;
					default:
						printDebug("Connection::handleRead: error reading data: ", error.message(), "\n");
						break;
					}
				}

			}

			void Connection::handleWrite(std::shared_ptr<Message> message, const boost::system::error_code & error, size_t len)
			{
				if (!error.failed())
				{
					printDebug("Sent message to client\n");
				}
				else
				{
					printDebug("Connection::handleWrite: error writing data: ", error.message(), "\n");
				}
			}

			void Connection::handleDisconnect()
			{
				printDebug("Connection::handleRead: error: client disconnected\n");
				disconnected(shared_from_this());
			}

			void Connection::handleClientReady()
			{
				printDebug("Connection: client is ready\n");
				readied(shared_from_this());
			}

			void Connection::handleTurnResult(FourAcross::TurnResult result, uint8_t column)
			{
				// send turn result to client
				std::shared_ptr<Message> message{new Message{}};
				message->type = MessageType::turnResult;
				message->data[0] = toUnderlyingType(result);
				message->data[1] = column;
				sendMessage(message);
			}

			void Connection::disconnect()
			{
				// tell lobby we're leaving
				disconnected(shared_from_this());
			}
		}
	}
}