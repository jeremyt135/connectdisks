#include "four-across/networking/server/server.hpp"

#include "four-across/networking/server/gamelobby.hpp"
#include "four-across/networking/server/connection.hpp"

#include "type-utility.hpp"
#include "logging.hpp"

#include <algorithm>
#include <iostream>
#include <functional>

using boost::asio::ip::address_v4;
using boost::asio::ip::tcp;

namespace game
{
	namespace networking
	{
		namespace server
		{
			Server::Server(
				boost::asio::io_service & ioService,
				std::string address, uint16_t port
			) :
				ioService{ioService},
				acceptor{ioService, tcp::endpoint{address_v4::from_string(address), port}}
			{
				waitForConnections();
			}

			Server::~Server()
			{
			}

			void Server::waitForConnections()
			{
				print("Server waiting for connections\n");

				std::shared_ptr<Connection> connection{Connection::create(ioService)};

				acceptor.async_accept(
					connection->getSocket(),
					std::bind(
						&Server::handleConnection, this,
						connection, std::placeholders::_1));
			}

			void Server::handleConnection(std::shared_ptr<Connection> connection, const boost::system::error_code & error)
			{
				print("Server trying to accept connection\n");

				if (!error.failed())
				{
					const auto numLobbies = lobbies.size();
					print("Server accepted connection, there are ", numLobbies, " lobbies \n");
					// assign connection to an existing lobby if one exists
					if (numLobbies != 0)
					{
						auto lobby = findAvailableLobby();
						if (lobby)
						{
							print("Adding player to existing lobby\n");
							lobby->addPlayer(connection);
						}
						else // all current lobbies are full
						{
							// only make new lobby if not at cap
							if (numLobbies < maxLobbies)
							{
								// make a new lobby and add player
								auto lobby = makeNewLobby();
								print("Adding player to new lobby\n");
								lobby->addPlayer(connection);
							}
							else
							{
								print("Server at lobby cap, not adding player\n");
							}
						}
					}
					else // no existing lobbies
					{
						if (numLobbies < maxLobbies)
						{
							auto lobby = makeNewLobby();
							print("Adding player to new lobby\n");
							lobby->addPlayer(connection);
						}
						else
						{
							print("Server at lobby cap, not adding player\n");
						}
					}
				}
				else
				{
					printDebug("Server::handleConnection: error accepting connection: ", error.message(), "\n");
					return;
				}

				print("Added player\n");

				waitForConnections();
			}

			GameLobby * Server::findAvailableLobby()
			{
				// find a lobby that isn't full
				auto lobby = std::find_if(
					lobbies.begin(),
					lobbies.end(),
					[](std::unique_ptr<GameLobby>& gameLobby){
						return !gameLobby->isFull();
					}
				);
				if (lobby != lobbies.end())
				{
					print("Found lobby for new player: ", std::distance(lobbies.begin(), lobby), "\n");
					return lobby->get();
				}
				return nullptr;
			}

			GameLobby * Server::makeNewLobby()
			{
				print("Making new lobby\n");
				lobbies.emplace_back(new GameLobby{}); // make a new lobby using default number of max players
				auto lobby = lobbies.back().get();
				lobby->start();
				return lobby;
			}
		}
	}
}