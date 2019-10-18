#include "four-across/networking/server/gamelobby.hpp"

#include "type-utility.hpp"

#include "logging.hpp"

#include <algorithm>
#include <iostream>
#include <functional>
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
				canAddPlayers{false},
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

			void GameLobby::start()
			{
				startLobby();
			}

			void GameLobby::startLobby()
			{
				lobbyIsOpen = true;
				canAddPlayers = true;
				print("GameLobby [", this, "]: has started\n");
			}

			void GameLobby::startGame()
			{
				isPlayingGame = true;
				canAddPlayers = false;

				gameStarted();
				takeTurn(game->getCurrentPlayer());
			}

			void GameLobby::stopGame()
			{
				// stop playing if lost a player
				print("GameLobby [", this, "]: is stopping game\n");
				numReady = 0;
				isPlayingGame = false;
				game.reset();
				gameEnded(game->noWinner);
			}

			void GameLobby::onGameOver()
			{
				// no longer playing a game but do not tell players game is over again
				if (game->hasWinner())
				{
					gameEnded(game->getWinner());
				}
				else
				{
					gameEnded(game->noWinner);
				}
				isPlayingGame = false;
				numReady = 0;
				game.reset();
			}

			void GameLobby::addPlayer(std::shared_ptr<Connection> connection)
			{
				if (!canAddPlayers)
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
					connection->addDisconnectHandler(bind(&GameLobby::onDisconnect, this, _1));
					connection->addReadyHandler(bind(&GameLobby::onReady, this, _1));
					connection->addTurnHandler(bind(&GameLobby::onTakeTurn, this, _1, _2));
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
					// start process of closing connection by releasing our handle of the shared_ptr
					playerIter->reset();

					if (numPlayers > 0)
					{
						--numPlayers;
					}
					canAddPlayers = true;

					lobbyAvailable(this);

					if (numReady > 0)
					{
						--numReady;
					}

					print("GameLobby[", this, "]: player disconnected; remaining: ", static_cast<int>(numPlayers), "\n");

					if (isPlayingGame)
					{
						stopGame();
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
						game.reset(new FourAcross{maxPlayers}); // use default that first player is id 1

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
		}
	}
}