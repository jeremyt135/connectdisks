#pragma once

#include "four-across/game/game.hpp"

#include <boost/asio.hpp>

#include <list>
#include <memory>
#include <string>
#include <vector>

namespace game
{
	namespace networking
	{
		namespace server
		{
			class GameLobby; // Runs a FourAcross game
			class Connection; // Maintains connection from client

			/*
				Accepts connections from clients wanting to play FourAcross
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
				static constexpr uint8_t maxLobbies{4};

				void waitForConnections();
				void onConnectionAccepted(std::shared_ptr<Connection> connection, const boost::system::error_code& error);

				void addToQueue(std::shared_ptr<Connection> connection);
				void updateQueuePositions(const boost::system::error_code& error, boost::asio::steady_timer* timer);

				GameLobby* findAvailableLobby();
				GameLobby* makeNewLobby();
				void onLobbyAvailable(GameLobby* lobby);

				boost::asio::io_service& ioService;
				boost::asio::ip::tcp::acceptor acceptor;
				boost::asio::steady_timer queueUpdateTimer;

				size_t lastQueueSize;
				std::list<std::shared_ptr<Connection>> playerQueue;
				std::vector<std::unique_ptr<GameLobby>> lobbies;
			};
		}
	}
}