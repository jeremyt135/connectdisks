#include "four-across/networking/client/client.hpp"

#include "four-across/networking/server/server.hpp"
#include "four-across/game/game.hpp"

#include "type-utility.hpp"
#include "logging.hpp"

#include <array>
#include <iostream>
#include <thread>

using boost::asio::ip::address_v4;
using boost::asio::ip::tcp;

using typeutil::toScopedEnum;

namespace game
{
	namespace client
	{
		Client::Client(boost::asio::io_service & ioService)
			:
			isPlaying{false},
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

		void Client::waitForMessages()
		{
			if (socket.is_open())
			{
				printDebug("Client waiting for messages\n");
				// read message from server
				std::shared_ptr<server::Message> message{new server::Message{}};
				boost::asio::async_read(socket,
					boost::asio::buffer(message.get(), sizeof(server::Message)),
					std::bind(
						&Client::handleRead,
						this,
						message,
						std::placeholders::_1,
						std::placeholders::_2
					));
			}
		}

		void Client::sendMessage(std::shared_ptr<client::Message> message)
		{
			boost::asio::async_write(socket,
				boost::asio::buffer(message.get(), sizeof(client::Message)),
				std::bind(
					&Client::handleWrite,
					this,
					message,
					std::placeholders::_1,
					std::placeholders::_2
				));
		}

		void Client::sendReady()
		{
			// tell server we're ready to play
			std::shared_ptr<client::Message> message{new client::Message{}};
			message->response = Response::ready;
			message->data[0] = playerId;
			sendMessage(message);
		}

		void Client::sendRematch(bool shouldRematch)
		{
			std::shared_ptr<client::Message> message{new client::Message{}};
			message->response = Response::rematch;
			message->data[0] = static_cast<bool>(true);
			message->data[1] = static_cast<bool>(shouldRematch);
			sendMessage(message);
		}

		void Client::handleConnection(const boost::system::error_code & error)
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

		void Client::handleRead(std::shared_ptr<server::Message> message, const boost::system::error_code & error, size_t len)
		{
			if (!error.failed())
			{
				printDebug("Client received message\n");

				if (len == 0)
				{
					return;
				}

				const auto response = message->response;

				switch (response)
				{
				case server::Response::connected:
					onConnected(message->data[0]);
					break;
				case server::Response::gameStart:
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
					// ignore if already playing
					if (!isPlaying)
					{
						onGameStarted(numPlayers, first, numCols, numRows);
					}
				}
				break;
				case server::Response::gameEnd:
				{
					printDebug("Client is in a game that ended; returned to lobby\n");
					const auto winner = message->data[0];

					onGameEnded(winner);
				}
				break;
				case server::Response::takeTurn:
					printDebug("Time for client to take turn\n");
					onTakeTurn();
					break;
				case server::Response::turnResult:
				{
					printDebug("Client received results of turn\n");
					const auto turnResult = toScopedEnum<FourAcross::TurnResult>::cast(message->data[0]);
					const auto column = message->data[1];
					onTurnResult(turnResult, column);
				}
				break;
				case server::Response::update:
				{
					printDebug("Client received an opponent's turn update\n");
					const auto player = message->data[0];
					const auto column = message->data[1];
					onUpdate(player, column);
				}
				break;
				case server::Response::rematch:
				{
					const auto hasStatus = static_cast<bool>(message->data[0]);
					if (!hasStatus)
					{
						// server is asking us if we want to rematch
						getRematchStatus();
					}
					else
					{
						// server knows if we wanted to rematch
						const auto shouldRematch = static_cast<bool>(message->data[1]);
						if (shouldRematch)
						{
							printDebug("Client received confirmation of rematch\n");
							// get new ready status
							getReadyStatus();
						}
						else if (socket.is_open())
						{
							// socket is open still (we didn't disconnect) but server responded with wrong value
							printDebug("Client received rematch confirmation but value was 0\n");
							// TODO: ask user again for rematch status, try to send to server again
							disconnect(); // for now handle by disconnecting
						}
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
					printDebug("Client::handleRead: connection ended \n");
					break;
				default:
					printDebug("Client::handleRead: unknown error occurred: ", error.message(), "\n");
					break;
				}

				onDisconnect();
			}
		}

		void Client::handleWrite(std::shared_ptr<client::Message> message, const boost::system::error_code & error, size_t len)
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

		void Client::getReadyStatus()
		{
			auto isReady = askIfReady();
			if (isReady.has_value())
			{
				if (isReady.get())
				{
					sendReady();
				}
				else
				{
					// user no longer wants to play
					disconnect();
				}
			}
			else
			{
				// if unassigned, there's no slot attached
				const auto errorMsg = "Client::getReadyStatus: error: no slot attached to signal askIfReady\n";
				printDebug(errorMsg);
				throw std::runtime_error(errorMsg);
			}
		}

		void Client::getRematchStatus()
		{
			auto wantsRematch = askIfRematch();
			if (wantsRematch.has_value())
			{
				if (wantsRematch.get())
				{
					sendRematch(true);
				}
				else
				{
					// user not playing again
					disconnect();
				}
			}
			else
			{
				// if unassigned, there's no slot attached
				const auto errorMsg = "Client::getRematchStatus: error: no slot attached to signal askIfRematch\n";
				printDebug(errorMsg);
				throw std::runtime_error(errorMsg);
			}
		}

		void Client::onConnected(uint8_t id)
		{
			playerId = id;
			printDebug("Client id set to: ", static_cast<int>(playerId), "\n");
			connected(id);
			getReadyStatus();
		}

		void Client::onDisconnect()
		{
			stopPlaying();
			disconnect();
		}

		void Client::onGameStarted(uint8_t numPlayers, uint8_t first, uint8_t cols, uint8_t rows)
		{
			game.reset(new FourAcross{numPlayers, first, cols, rows});
			startPlaying();
			gameStarted(numPlayers, first, cols, rows);
		}

		void Client::onGameEnded(uint8_t winner)
		{
			stopPlaying();
			gameEnded(winner);
		}

		void Client::sendTurn(uint8_t column)
		{
			// send turn to server
			std::shared_ptr<client::Message> message{new client::Message{}};
			message->response = Response::turn;
			message->data[0] = column;
			boost::asio::async_write(socket,
				boost::asio::buffer(message.get(), sizeof(client::Message)),
				std::bind(
					&Client::handleWrite,
					this,
					message,
					std::placeholders::_1,
					std::placeholders::_2
				));
		}

		void Client::onTakeTurn()
		{
			auto column = takeTurn(); // result of signal with value is boost::optional
			if (column)
			{
				sendTurn(column.get());
			}
			else
			{
				// if unassigned, there's no slot attached
				const auto errorMsg = "Client::onTakeTurn: error: no slot attached to signal takeTurn\n";
				printDebug(errorMsg);
				throw std::runtime_error(errorMsg);
			}
		}

		void Client::onTurnResult(FourAcross::TurnResult result, uint8_t column)
		{
			switch (result)
			{
			case FourAcross::TurnResult::success:
				game->takeTurn(playerId, column);
				tookTurn(result, column);
				break;
			case FourAcross::TurnResult::error:
				printDebug("Client::onTurnResult: error taking turn, not requesting a new turn unless server asks\n");
				break;
			default:
				printDebug("Client::onTurnResult: Unsuccessful turn, client sending new turn\n");
				tookTurn(result, column);
				onTakeTurn();
				break;
			}

		}

		void Client::onUpdate(uint8_t player, uint8_t col)
		{
			// assume all turns sent to us from server are good
			game->takeTurn(player, col);

			gameUpdated(player, col);
		}

		const FourAcross * Client::getGame() const noexcept
		{
			return game.get();
		}

		void Client::startPlaying()
		{
			printDebug("Client is playing\n");
			isPlaying = true;
		}

		void Client::stopPlaying()
		{
			printDebug("Client has stopped playing\n");
			isPlaying = false;
		}

		void Client::disconnect()
		{
			disconnected();
			boost::system::error_code error;
			socket.shutdown(tcp::socket::shutdown_both, error);
			if (!error)
			{
				socket.close();
			}
			else
			{
				printDebug("Client::disconnect: error closing socket: ", error.message(), "\n");
			}
		}

	}
}
