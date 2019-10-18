#include "four-across/networking/server/gamelobby.hpp"

#include "type-utility.hpp"

#include "logging.hpp"

#include <algorithm>
#include <iostream>
#include <functional>
#include <random>
#include <utility>

using boost::asio::ip::address_v4;
using boost::asio::ip::tcp;

namespace game
{
	namespace networking
	{
		namespace server
		{
			GameLobby::GameLobby(uint8_t maxPlayers) :
				lobbyIsOpen{false},
				isPlayingGame{false},
				maxPlayers{maxPlayers},
				numReady{0},
				numPlayers{0}
			{
				players.resize(maxPlayers);
			}

			GameLobby::~GameLobby()
			{
			}

			void GameLobby::addPlayer(std::shared_ptr<Connection> connection)
			{
				if (!canAddPlayers())
				{
					return;
				}

				const auto id = getFirstAvailableId();

				players[id] = connection;
				players[id]->setId(id + 1);
				players[id]->setGameLobby(this);

				{
					using std::placeholders::_1;
					using std::placeholders::_2;
					using std::bind;
					players[id]->addDisconnectHandler(bind(&GameLobby::onDisconnect, this, _1));
					players[id]->addReadyHandler(bind(&GameLobby::onReady, this, _1));
					players[id]->addTurnHandler(bind(&GameLobby::onTakeTurn, this, _1, _2));
				}

				++numPlayers;
			}

			uint8_t GameLobby::getFirstAvailableId() const
			{
				// find shared_ptr<Connection> holding a nullptr
				const auto iter = std::find_if(players.begin(), players.end(), [](std::shared_ptr<Connection> con){ return con == nullptr; });
				const auto index = std::distance(players.begin(), iter);
				return static_cast<uint8_t>(index);
			}

			void GameLobby::start()
			{
				startLobby();
			}

			void GameLobby::startLobby()
			{
				if (lobbyIsOpen)
				{
					return;
				}
				print("GameLobby [", this, "]: has started\n");
				lobbyIsOpen = true;
			}

			void GameLobby::startGame()
			{
				if (lobbyIsOpen && !isPlayingGame)
				{
					isPlayingGame = true;

					gameStarted();
					takeTurn(game->getCurrentPlayer());
				}
			}

			void GameLobby::onGameOver()
			{
				print("GameLobby [", this, "]: is stopping game\n");

				// game ended, notify connections with winner, if there is one
				if (game->hasWinner())
				{
					gameEnded(game->getWinner());
				}
				else
				{
					gameEnded(game->noWinner);
				}

				game.reset();

				isPlayingGame = false;
				numReady = 0;
			}

			void GameLobby::onDisconnect(std::shared_ptr<Connection> connection)
			{
				// find the player that disconnected
				auto playerIter = std::find_if(players.begin(), players.end(),
					[connection](std::shared_ptr<Connection> con){
						if (con)
						{
							return con->getId() == connection->getId();
						}
						return false;
					});
				if (playerIter != players.end())
				{
					auto playerWasReady = (*playerIter)->isReady();
					if (playerWasReady && numReady > 0)
					{
						--numReady;
					}

					// start process of closing connection by releasing our handle of the shared_ptr
					playerIter->reset();
					if (numPlayers > 0)
					{
						--numPlayers;
					}

					print("GameLobby[", this, "]: player disconnected; remaining: ", static_cast<int>(numPlayers), "\n");

					if (isPlayingGame)
					{
						//stopGame();
						onGameOver();
					}

					if (canAddPlayers())
					{
						lobbyAvailable(this);
					}
				}
				else
				{
					// didn't find id in lobby
					printDebug("GameLobby[", this, "]: player disconnected, but didn't find ID in lobby\n");
				}
			}

			void GameLobby::onReady(std::shared_ptr<Connection> connection)
			{
				// find the connection in this lobby that has same id as the one that just readied
				auto player = std::find_if(players.begin(), players.end(),
					[connection](std::shared_ptr<Connection> con){
						if (con != nullptr)
						{
							return con->getId() == connection->getId();
						}
						return false;
					});
				if (player != players.end())
				{
					++numReady;
					if (allPlayersAreReady() && isFull())
					{
						// randomly pick first player
						std::random_device seed;
						std::default_random_engine engine(seed());
						std::uniform_int_distribution<unsigned short> dist(1u, static_cast<unsigned short>(maxPlayers));
						auto first = static_cast<uint8_t>(dist(engine));

						game.reset(new FourAcross{maxPlayers, first});

						// start if all players are ready
						startGame();
					}
				}
			}

			void GameLobby::onTakeTurn(std::shared_ptr<Connection> connection, uint8_t column)
			{
				if (connection == nullptr || !isPlayingGame)
				{
					tookTurn(connection->getId(), column, FourAcross::TurnResult::error);
					return;
				}

				try
				{
					const auto result = game->takeTurn(connection->getId(), column);

					// notify other players that there was a move
					tookTurn(connection->getId(), column, result);

					if (game->hasWinner() || game->boardFull())
					{
						onGameOver();
					}
					else if (result == FourAcross::TurnResult::success)
					{
						// if turn was successful, tell next player to take turn
						takeTurn(game->getCurrentPlayer());
					}
				}
				catch (std::exception& error)
				{
					printDebug("GameLobby[", this, "]::onTakeTurn: error taking turn: ", error.what(), "\n");
					tookTurn(connection->getId(), column, FourAcross::TurnResult::error);
				}
			}

			bool GameLobby::isEmpty() const noexcept
			{
				return players.empty();
			}

			bool GameLobby::isFull() const noexcept
			{
				return numPlayers == maxPlayers;
			}

			uint8_t GameLobby::getNumPlayers() const noexcept
			{
				return static_cast<uint8_t>(numPlayers);
			}

			FourAcross * GameLobby::getGame() const noexcept
			{
				return game.get();
			}

			bool GameLobby::allPlayersAreReady() const noexcept
			{
				return numReady == numPlayers;
			}
			bool GameLobby::canAddPlayers() const noexcept
			{
				return lobbyIsOpen && !isPlayingGame && !isFull();
			}
		}
	}
}