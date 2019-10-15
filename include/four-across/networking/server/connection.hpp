#pragma once

#include "four-across/game/game.hpp"
#include "four-across/networking/messaging.hpp"

#include "signals-helper.hpp"

#include <boost/asio.hpp>

#include <memory>
#include <string>
#include <vector>

namespace game
{
	namespace networking
	{
		namespace server
		{
			class GameLobby;

			/*
				Maintains connection from clients and handles socket I/O.
			*/
			class Connection : public std::enable_shared_from_this<Connection>
			{
				ADD_SIGNAL(Disconnect, disconnected, void, std::shared_ptr<Connection>)
					ADD_SIGNAL(Ready, readied, void, std::shared_ptr<Connection>)
					ADD_SIGNAL(Turn, tookTurn, void, std::shared_ptr<Connection>, uint8_t)

			public:
				static std::shared_ptr<Connection> create(boost::asio::io_service& ioService, GameLobby* lobby = nullptr);

				// Starts an async read from the socket
				void waitForMessages();

				// Sets the id that the player should have
				void setId(uint8_t id);

				uint8_t getId() const noexcept;

				// Sets the GameLobby that this is connected to
				void setGameLobby(GameLobby* lobby);

				boost::asio::ip::tcp::socket& getSocket();
			private:
				Connection(boost::asio::io_service& ioService, GameLobby* lobby = nullptr);

				void onTurn(uint8_t playerId);
				void onGameStart();
				void onGameEnd(uint8_t winner);
				void onUpdate(uint8_t playerId, uint8_t col, FourAcross::TurnResult result);

				// Sends a message to client
				void sendMessage(std::shared_ptr<Message> message);

				void sendWinner(uint8_t winner);

				void disconnect();

				// Handles messages from the client on other end of connection
				void handleRead(std::shared_ptr<Message> message, const boost::system::error_code& error, size_t len);

				// Handles result of sending message to client
				void handleWrite(std::shared_ptr<Message> message, const  boost::system::error_code& error, size_t len);

				void handleDisconnect();
				void handleClientReady();
				void handleTurnResult(FourAcross::TurnResult result, uint8_t column);

				GameLobby* lobby;

				boost::asio::ip::tcp::socket socket;
				uint8_t id;
			};
		}
	}
}