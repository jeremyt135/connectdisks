#include "connectdisks/gamelobby.hpp"

#include "type_utility.hpp"

#include "logging.hpp"

#include <boost/endian/arithmetic.hpp>
#include <boost/endian/conversion.hpp>

#include <algorithm>
#include <iostream>
#include <functional>
#include <utility>

using boost::asio::ip::address_v4;
using boost::asio::ip::tcp;

using namespace connectdisks::server;
using connectdisks::Board;
using connectdisks::ConnectDisks;

using typeutil::toUnderlyingType;
using typeutil::toScopedEnum;

connectdisks::server::GameLobby::GameLobby(Board::player_size_t maxPlayers) :
	lobbyIsOpen{false},
	canAddPlayers{true},
	isPlayingGame{false},
	maxPlayers{maxPlayers},
	numReady{0},
	numPlayers{0}
{
	players.resize(maxPlayers);
}

connectdisks::server::GameLobby::~GameLobby()
{
}

void connectdisks::server::GameLobby::start()
{
	startLobby();
}

void connectdisks::server::GameLobby::onDisconnect(std::shared_ptr<Connection> connection)
{
	if (!lobbyIsOpen)
	{
		return;
	}

	std::lock_guard<std::mutex> lock{playersMutex};

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
		// reset connection pointer
		playerIter->reset();

		--numPlayers;
		--numReady;

		print("GameLobby[", this, "]: player disconnected; remaining: ", static_cast<int>(numPlayers), "\n");
		if (isPlayingGame)
		{
			stopGame();
		}

		// todo handle game over plus empty lobby
		if (isEmptyInternal())
		{
			game.reset();
		}
	}

}

void connectdisks::server::GameLobby::onReady(std::shared_ptr<Connection> connection)
{
	std::unique_lock<std::mutex> lock{playersMutex};
	auto player = std::find_if(players.begin(), players.end(),
		[connection](std::shared_ptr<Connection> con){
			return con->getId() == connection->getId();
		});
	if (player != players.end())
	{
		// connection belongs to lobby

		++numReady;
		if (allPlayersAreReady() && isFullInternal())
		{
			//Board::player_size_t id = 1;
			//std::for_each(players.begin(), players.end(),
			//	[&id, this](std::shared_ptr<Connection> con){
			//		gameIds[con->getId()] = id++; // map player ids to game ids
			//	}
			//);
			game.reset(new ConnectDisks{maxPlayers}); // use default that first player is id 1
			lock.unlock();
			// start if all players are ready
			startGame();
		}
	}
}

ConnectDisks::TurnResult connectdisks::server::GameLobby::onTakeTurn(std::shared_ptr<Connection> connection, Board::board_size_t column)
{
	if (connection == nullptr)
	{
		return ConnectDisks::TurnResult::error;
	}

	std::lock_guard<std::mutex> lock{playersMutex};
	try
	{
		const auto result = game->takeTurn(connection->getId(), column);
		switch (result)
		{
		case ConnectDisks::TurnResult::success:
		{
			// tell other clients that there was a move
			std::for_each(players.begin(), players.end(),
				[connection, column, result, this](std::shared_ptr<Connection> otherConnection){
					if (otherConnection != nullptr)
					{
						if (connection->getId() != otherConnection->getId())
						{
							otherConnection->onUpdate(connection->getId(), column);
						}
						if (game->hasWinner())
						{
							otherConnection->onGameEnd(game->getWinner());
						}
						else if (game->boardFull())
						{
							otherConnection->onGameEnd(0);
						}
						else if (otherConnection->getId() == game->getCurrentPlayer())
						{
							// tell the next player it's their turn
							otherConnection->onTurn();
						}
					}
				});
			if (game->hasWinner())
			{
				onGameOver();
			}
			else if (game->boardFull())
			{
				onGameOver();
			}
		}
		break;
		default:
			break;
		}
		return result;
	}
	catch (std::exception& error)
	{
		printDebug("GameLobby[", this, "]::onTakeTurn: error taking turn: ", error.what(), "\n");
	}

	return ConnectDisks::TurnResult::error;
}

void connectdisks::server::GameLobby::startGame()
{
	// clumsy - fix with atomic memory order
	std::unique_lock<std::mutex> lock{playersMutex};
	canAddPlayers = false;
	isPlayingGame = true;
	lock.unlock();

	// should now be immutable
	for (auto& player : players)
	{
		if (player)
		{
			player->onGameStart();
			if (player->getId() == game->getCurrentPlayer())
			{
				player->onTurn();
			}
		}
	}
}

void connectdisks::server::GameLobby::stopGame()
{
	print("GameLobby [", this, "]: is stopping game\n");
	// stop playing if lost a player
	isPlayingGame = false;
	for (auto& player : players)
	{
		if (player)
		{
			player->onGameEnd(0);
		}
	}
}

void connectdisks::server::GameLobby::addPlayer(std::shared_ptr<Connection> connection)
{
	std::lock_guard<std::mutex> lock{playersMutex};

	const auto id = getFirstAvailableId();

	players[id] = connection;

	players[id]->setId(id + 1);
	players[id]->setGameLobby(this);
	players[id]->waitForMessages();

	++numPlayers;
}

bool connectdisks::server::GameLobby::isEmpty() const noexcept
{
	std::lock_guard<std::mutex> lock(playersMutex);
	return isEmptyInternal();
}

bool connectdisks::server::GameLobby::isFull() const noexcept
{
	std::lock_guard<std::mutex> lock(playersMutex);
	return isFullInternal();
}

Board::player_size_t connectdisks::server::GameLobby::getNumPlayers() const noexcept
{
	std::lock_guard<std::mutex> lock{playersMutex};
	return static_cast<Board::player_size_t>(numPlayers);
}

ConnectDisks * connectdisks::server::GameLobby::getGame() const noexcept
{
	return game.get();
}

void connectdisks::server::GameLobby::startLobby()
{
	lobbyIsOpen = true;
	print("GameLobby [", this, "]: has started\n");
}

void connectdisks::server::GameLobby::onGameOver()
{
	// no longer playing a game but do not tell players game is over again
	isPlayingGame = false;
}

bool connectdisks::server::GameLobby::isEmptyInternal() const noexcept
{
	return players.empty();
}

bool connectdisks::server::GameLobby::isFullInternal() const noexcept
{
	return numPlayers == maxPlayers;
}

bool connectdisks::server::GameLobby::allPlayersAreReady() const noexcept
{
	return numReady == numPlayers;
}

Board::player_size_t connectdisks::server::GameLobby::getFirstAvailableId() const
{
	// find shared_ptr<Connection> holding a nullptr
	const auto iter = std::find_if(players.begin(), players.end(), [](std::shared_ptr<Connection> con){ return con == nullptr; });
	const auto index = std::distance(players.begin(), iter);
	return static_cast<Board::player_size_t>(index);
}

