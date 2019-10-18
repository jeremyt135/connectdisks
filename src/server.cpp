#include "four-across/networking/server/server.hpp"

#include "four-across/networking/server/gamelobby.hpp"
#include "four-across/networking/server/connection.hpp"

#include "type-utility.hpp"
#include "logging.hpp"

#include <algorithm>
#include <cmath>
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
				acceptor{ioService, tcp::endpoint{address_v4::from_string(address), port}},
				queueUpdateTimer{ioService},
				lastQueueSize{0}
			{
				waitForConnections();
				queueUpdateTimer.expires_from_now(boost::asio::chrono::seconds(30));
				queueUpdateTimer.async_wait(std::bind(&Server::updateQueuePositions, this, std::placeholders::_1, &queueUpdateTimer));
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
						&Server::onConnectionAccepted, this,
						connection, std::placeholders::_1));
			}

			void Server::onConnectionAccepted(std::shared_ptr<Connection> connection, const boost::system::error_code & error)
			{
				print("Server trying to accept connection\n");

				if (!error.failed())
				{
					connection->onAccept();

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
								print("Server at lobby cap, adding player to queue\n");
								addToQueue(connection);
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
							print("Server at lobby cap, adding player to queue\n");
							addToQueue(connection);
						}
					}
				}
				else
				{
					printDebug("Server::handleConnection: error accepting connection: ", error.message(), "\n");
					return;
				}

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
				lobby->addLobbyAvailableHandler(std::bind(&Server::onLobbyAvailable, this, std::placeholders::_1));
				lobby->start();
				return lobby;
			}

			void Server::onLobbyAvailable(GameLobby * lobby)
			{
				if (playerQueue.size() > 0)
				{
					auto player = playerQueue.front();
					lobby->addPlayer(player);
					playerQueue.pop_front();
				}
			}

			void Server::addToQueue(std::shared_ptr<Connection> connection)
			{
				playerQueue.push_back(connection);
				connection->notifyQueuePosition(playerQueue.size());
			}

			void Server::updateQueuePositions(const boost::system::error_code & error, boost::asio::steady_timer * timer)
			{
				// remove players that have closed their connection
				playerQueue.remove_if([](std::shared_ptr<Connection> connection){ return !connection->isAlive(); });

				// notify remaining players
				auto queueSize = playerQueue.size();
				auto delta = queueSize > lastQueueSize ? queueSize - lastQueueSize : lastQueueSize - queueSize;
				if (queueSize > 0 && (queueSize < 10 || delta >= 10))
				{
					size_t pos{1};
					for (auto playerIter = playerQueue.begin(); playerIter != playerQueue.end(); ++playerIter, ++pos)
					{
						auto player = *playerIter;
						player->notifyQueuePosition(pos);
					}
				}
				lastQueueSize = queueSize;

				// set timer to go again a minute from now
				timer->expires_from_now(boost::asio::chrono::seconds(30));
				timer->async_wait(std::bind(&Server::updateQueuePositions, this, std::placeholders::_1, timer));
			}
		}
	}
}