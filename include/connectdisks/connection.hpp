#pragma once

#include "connectdisks/board.hpp"
#include "connectdisks/connectdisks.hpp"

#include "connectdisks/messaging.hpp"

#include <boost/asio.hpp>

#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace connectdisks
{
	namespace server
	{
		class GameLobby;

		// Maintains connection from clients
		class Connection : public std::enable_shared_from_this<Connection>
		{
		public:
			// TODO - Finalize passing method/subscription for disconnecting from lobby
			static std::shared_ptr<Connection> create(boost::asio::io_service& ioService, GameLobby* lobby = nullptr);

			// TODO - Finalize passing method/subscription for disconnecting from lobby
			void onGameStart();
			void onGameEnd();

			// Starts an async read from the socket
			void waitForMessages();

			// Sets the id that the player should have
			void setId(Board::player_size_t id);

			Board::player_size_t getId() const noexcept;

			// Sets the GameLobby that this is connected to
			void setGameLobby(GameLobby* lobby);

			boost::asio::ip::tcp::socket& getSocket();
		private:
			Connection(boost::asio::io_service& ioService, GameLobby* lobby = nullptr);

			// Handles messages from the client on other end of connection
			void handleRead(std::shared_ptr<client::Message> message, const boost::system::error_code& error, size_t len);

			// Sends message to client
			void handleWrite(std::shared_ptr<server::Message> message, const  boost::system::error_code& error, size_t len);

			void handleDisconnect();
			void handleClientReady();

			GameLobby* lobby;

			boost::asio::ip::tcp::socket socket;
			Board::player_size_t id;
		};
	}
}