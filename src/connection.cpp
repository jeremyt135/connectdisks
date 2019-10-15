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
	namespace server
	{

		std::shared_ptr<Connection> Connection::create(boost::asio::io_service & ioService, GameLobby* lobby)
		{
			return std::shared_ptr<Connection>{new Connection{ioService, lobby}};
		}

		void Connection::onGameStart()
		{
			// send id to client
			std::shared_ptr<server::Message> response{new server::Message{}};
			response->response = Response::gameStart;

			// send the number of players
			response->data[0] = lobby->getNumPlayers();

			// send the board dimensions
			auto* game = lobby->getGame();

			response->data[1] = game->getNumColumns();
			response->data[2] = game->getNumRows();
			response->data[3] = game->getCurrentPlayer();

			sendMessage(response);
		}

		void Connection::onGameEnd(uint8_t player)
		{
			sendWinner(player);
			askForRematch();
		}

		void Connection::sendWinner(uint8_t winner)
		{
			// tell client game has ended and which player won
			std::shared_ptr<server::Message> message{new server::Message{}};
			message->response = Response::gameEnd;
			message->data[0] = winner;
			sendMessage(message);
		}

		void Connection::askForRematch()
		{
			std::shared_ptr<server::Message> message{new server::Message{}};
			message->response = Response::rematch;
			message->data[0] = static_cast<uint8_t>(false);
			sendMessage(message);
		}

		void Connection::waitForMessages()
		{
			printDebug("Connection waiting to read message\n");
			// read a message from the client, handle in handleRead
			std::shared_ptr<client::Message> message{new client::Message{}};
			boost::asio::async_read(socket,
				boost::asio::buffer(message.get(), sizeof(client::Message)),
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
				std::shared_ptr<server::Message> message{new server::Message{}};
				message->response = Response::connected;
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
				std::shared_ptr<server::Message> response{new server::Message{}};
				response->response = Response::takeTurn;
				sendMessage(response);
			}
		}


		void Connection::onUpdate(uint8_t playerId, uint8_t col, FourAcross::TurnResult result)
		{

			if (playerId != id)
			{
				if (result == FourAcross::TurnResult::success)
				{
					// tell client that a successful turn was taken by another player
					std::shared_ptr<server::Message> message{new server::Message{}};
					message->response = Response::update;
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

		void Connection::sendMessage(std::shared_ptr<server::Message> message)
		{
			boost::asio::async_write(socket,
				boost::asio::buffer(message.get(), sizeof(server::Message)),
				std::bind(
					&Connection::handleWrite,
					this,
					message,
					std::placeholders::_1,
					std::placeholders::_2
				));
		}

		void Connection::handleRead(std::shared_ptr<client::Message> message, const boost::system::error_code & error, size_t len)
		{
			printDebug("Connection trying to read message\n");
			if (!error.failed())
			{

				if (len == 0)
				{
					printDebug("Connection received 0 length message\n");
					return;
				}

				const auto response = message->response;
				switch (response)
				{
				case client::Response::ready:
					handleClientReady();
					break;
				case client::Response::turn:
				{
					const auto column = message->data[0];
					printDebug("Connection: client wants to move in column: ", static_cast<int>(column), "\n");
					tookTurn(shared_from_this(), column);
				}
				break;
				case client::Response::rematch:
				{
					const auto hasStatus = static_cast<bool>(message->data[0]);
					if (hasStatus)
					{
						// client is responding with their rematch status
						const auto shouldRematch = static_cast<bool>(message->data[1]);
						handleRematch(shouldRematch);
					}
					else
					{
						// client responded but without a status
						printDebug("Connection received rematch message with empty status\n");
						// TODO: ask client to send again
						disconnect();
					}
				}
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

		void Connection::handleWrite(std::shared_ptr<server::Message> message, const boost::system::error_code & error, size_t len)
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
			std::shared_ptr<server::Message> message{new server::Message{}};
			message->response = Response::turnResult;
			message->data[0] = toUnderlyingType(result);
			message->data[1] = column;
			sendMessage(message);
		}

		void Connection::handleRematch(bool shouldRematch)
		{
			printDebug("Connection: client wants rematch?: ", shouldRematch, "\n");
			if (shouldRematch)
			{
				confirmRematch();
			}
			else
			{
				disconnect();
			}
		}

		void Connection::confirmRematch()
		{
			// echo to client that they're rematching
			std::shared_ptr<server::Message> message{new server::Message{}};
			message->response = Response::rematch;
			message->data[0] = static_cast<uint8_t>(true);
			message->data[1] = static_cast<uint8_t>(true);
			sendMessage(message);
		}

		void Connection::disconnect()
		{
			// tell lobby we're leaving
			disconnected(shared_from_this());
		}


	}
}