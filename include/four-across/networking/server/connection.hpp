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

				// Starts an async read from the socket.
				void waitForMessages();

				// Sets the game id that the player should have. The id has no meaning outside of a GameLobby.
				void setId(uint8_t id);

				// Call from Server async_accept handler to notify this is alive.
				void onAccept();

				// Returns true if the client on other end of connection is verifiably still connected.
				bool isAlive() const noexcept;

				// Returns true if the client has readied up. This does not indicate if the connection
				// is still alive and should not be used as such.
				bool isReady() const noexcept;

				uint8_t getId() const noexcept;

				// Sets the GameLobby that this is connected to
				void setGameLobby(GameLobby* lobby);

				// Notifies Connection of their position in queue
				void notifyQueuePosition(uint64_t position);

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

				// Handles messages from the client on other end of connection
				void onReadSocket(std::shared_ptr<Message> message, const boost::system::error_code& error, size_t len);

				// Handles result of sending message to client
				void onWriteSocket(std::shared_ptr<Message> message, const  boost::system::error_code& error, size_t len);

				void handleDisconnect();
				void handleClientReady();
				void handleTurnResult(FourAcross::TurnResult result, uint8_t column);

				// Pings and checks if received pong from Client
				void pingOnTimer(const boost::system::error_code& error, boost::asio::steady_timer* timer);
				void sendPing();
				void startPings();

				boost::asio::ip::tcp::socket socket;
				boost::asio::steady_timer pingTimer;

				GameLobby* lobby;

				uint8_t id;

				bool clientIsConnected; // is client connection alive (are we receiving pings)
				bool clientIsReady; // did the client ready up? (for now this is only reset on game end)

				bool receivedPong;
				uint8_t missedPongs;
			};
		}
	}
}