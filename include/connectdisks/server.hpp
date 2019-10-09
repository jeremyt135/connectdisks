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
		class GameLobby; // Runs a ConnectDisks game
		class Connection; // Maintains connection from client

		/* 
			Accepts connections from clients wanting to play ConnectDisks 
			and manages game lobbies.
		 */
		class Server
		{
		public:
			Server(
				boost::asio::io_service& ioService,
				std::string address,
				uint16_t port);
			Server(const Server&) = delete;
			~Server();

			Server& operator=(const Server&) = delete;

		private:
			// limit is mostly arbitrary, but should probably be small unless lobbies poll i/o on their own thread
			static constexpr Board::player_size_t maxLobbies{4}; 

			void waitForConnections();
			void handleConnection(std::shared_ptr<Connection> connection, const boost::system::error_code& error);

			GameLobby* findAvailableLobby();
			GameLobby* makeNewLobby();

			boost::asio::io_service& ioService;
			boost::asio::ip::tcp::acceptor acceptor;

			std::vector<std::unique_ptr<GameLobby>> lobbies;
		};
	}
}
