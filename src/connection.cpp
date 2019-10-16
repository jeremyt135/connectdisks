#include "four-across/networking/server/connection.hpp"
#include "four-across/networking/server/gamelobby.hpp"

#include "type-utility.hpp"
#include "logging.hpp"

#include <boost/endian/conversion.hpp>

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
				if (!clientIsConnected)
				{
					return;
				}

				printDebug("Connection waiting to read message\n");
				// read a message from the client, handle in handleRead
				std::shared_ptr<Message> message{new Message{}};
				boost::asio::async_read(socket,
					boost::asio::buffer(message.get(), sizeof(Message)),
					std::bind(
						&Connection::onReadSocket,
						shared_from_this(),
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

			void Connection::onAccept()
			{
				waitForMessages();
				startPings();
			}

			bool Connection::isAlive() const noexcept
			{
				return clientIsConnected;
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
				lobby->addTurnHandler(
					GameLobby::TurnHandler(bind(&Connection::onTurn, shared_from_this(), _1)).track_foreign(shared_from_this()));
				lobby->addTurnResultHandler(
					GameLobby::TurnResultHandler(bind(&Connection::onUpdate, shared_from_this(), _1, _2, _3)).track_foreign(shared_from_this()));
				lobby->addGameStartHandler(
					GameLobby::GameStartHandler(bind(&Connection::onGameStart, shared_from_this())).track_foreign(shared_from_this()));
				lobby->addGameEndHandler(
					GameLobby::GameEndHandler(bind(&Connection::onGameEnd, shared_from_this(), _1)).track_foreign(shared_from_this()));
			}

			void Connection::notifyQueuePosition(uint64_t position)
			{
				std::shared_ptr<Message> message{new Message{}};
				boost::endian::native_to_big_inplace(position);
				message->type = MessageType::inQueue;
				memcpy(&message->data[0], &position, sizeof(uint64_t));
				sendMessage(message);
			}

			boost::asio::ip::tcp::socket& Connection::getSocket()
			{
				return socket;
			}

			Connection::Connection(boost::asio::io_service & ioService, GameLobby* lobby) :
				lobby{lobby},
				socket{ioService},
				pingTimer{ioService},
				id{0},
				clientIsConnected{true},
				receivedPong{false},
				missedPongs{0}
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
				if (!clientIsConnected)
				{
					return;
				}

				boost::asio::async_write(socket,
					boost::asio::buffer(message.get(), sizeof(Message)),
					std::bind(
						&Connection::onWriteSocket,
						shared_from_this(),
						message,
						std::placeholders::_1,
						std::placeholders::_2
					));
			}

			void Connection::onReadSocket(std::shared_ptr<Message> message, const boost::system::error_code & error, size_t len)
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
					case MessageType::pong:
						printDebug("PONG\n");
						receivedPong = true;
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

			void Connection::onWriteSocket(std::shared_ptr<Message> message, const boost::system::error_code & error, size_t len)
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
				if (clientIsConnected)
				{
					printDebug("Connection::handleRead: error: client disconnected\n");
					clientIsConnected = false;
					disconnected(shared_from_this());
				}
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

			void Connection::pingOnTimer(const boost::system::error_code & error, boost::asio::steady_timer * timer)
			{
				if (error)
				{
					printDebug("Connection::pingClient error: ", error.message(), "\n");
					return;
				}
				if (clientIsConnected)
				{
					if (receivedPong)
					{
						receivedPong = false;
						missedPongs = 0; // misses don't matter if alive

						// received last pong, send another
						sendPing();
						timer->expires_from_now(boost::asio::chrono::seconds(10));
						timer->async_wait(std::bind(&Connection::pingOnTimer, shared_from_this(), std::placeholders::_1, timer));
					}
					else
					{
						printDebug("Connection did not receive PONG\n");
						++missedPongs;
						if (missedPongs > 3)
						{
							// did not receive any pong, assume client is lost
							handleDisconnect();
						}
						else
						{
							sendPing();
							timer->expires_from_now(boost::asio::chrono::seconds(10));
							timer->async_wait(std::bind(&Connection::pingOnTimer, shared_from_this(), std::placeholders::_1, timer));
						}
					}
				}
			}
			void Connection::sendPing()
			{
				printDebug("PING\n");
				std::shared_ptr<Message> message{new Message{}};
				message->type = MessageType::ping;
				sendMessage(message);
			}
			void Connection::startPings()
			{
				sendPing();
				pingTimer.expires_from_now(boost::asio::chrono::seconds(10));
				pingTimer.async_wait(std::bind(&Connection::pingOnTimer, shared_from_this(), std::placeholders::_1, &pingTimer));
			}
		}
	}
}