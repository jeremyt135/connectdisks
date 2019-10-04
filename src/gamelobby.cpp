#include "connectdisks/gamelobby.hpp"

#include "type_utility.hpp"

#include <boost/endian/arithmetic.hpp>
#include <boost/endian/conversion.hpp>

#include <algorithm>
#include <iostream>
#include <functional>

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
	nextId{1},
	numReady{0}
{
	players.reserve(maxPlayers);
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
	size_t prevSize = players.size();
	players.erase(std::remove_if(players.begin(), players.end(),
		[connection](std::shared_ptr<Connection> con){ return con->getId() == connection->getId(); }));

	if (players.size() != prevSize)
	{
		// connection belongs to lobby
	#if defined DEBUG || defined _DEBUG
		std::cout << "GameLobby " << this << " player disconnected; remaining: " << players.size() << "\n";
	#endif
		--numReady;
		if (isPlayingGame)
		{
			stopGame();
		}

		// todo handle game over plus empty lobby
		/*if (isEmptyInternal())
		{
		}*/
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
		++numReady;
		// connection belongs to lobby
		if (allPlayersAreReady() && isFullInternal())
		{
			game.reset(new ConnectDisks{maxPlayers});
			lock.unlock();
			// start if all players are ready
			startGame();
		}
	}
}

ConnectDisks::TurnResult connectdisks::server::GameLobby::onTakeTurn(std::shared_ptr<Connection> connection, Board::board_size_t column)
{
	try
	{
		const auto result = game->takeTurn(connection->getId(), column);
		switch (result)
		{
		case ConnectDisks::TurnResult::success:
		{
			// tell other clients that there was a move
			std::lock_guard<std::mutex> lock{playersMutex};
			std::for_each(players.begin(), players.end(),
				[connection, column, result, this](std::shared_ptr<Connection> otherConnection){
					if (connection->getId() != otherConnection->getId())
					{
						otherConnection->onUpdate(otherConnection->getId(), column);
					}
					if (game->hasWinner())
					{
						otherConnection->onGameEnd(game->getWinner());
					}
					else if (otherConnection->getId() == game->getCurrentPlayer())
					{
						// tell the next player it's their turn
						otherConnection->onTurn();
					}
				});
			if (game->hasWinner())
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
	catch (std::exception& e)
	{
	#if defined DEBUG || defined _DEBUG
		std::cout << "GameLobby " << this << " error taking turn " << e.what() << "\n";
	#endif
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
		player->onGameStart();
		if (player->getId() == game->getCurrentPlayer())
		{
			player->onTurn();
		}
	}
}

void connectdisks::server::GameLobby::stopGame()
{
#if defined DEBUG || defined _DEBUG
	std::cout << "GameLobby " << this << " is stopping game\n";
#endif
	// stop playing if lost a player
	isPlayingGame = false;
	for (auto& player : players)
	{
		player->onGameEnd(0);
	}
}

void connectdisks::server::GameLobby::addPlayer(std::shared_ptr<Connection> connection)
{
	std::lock_guard<std::mutex> lock{playersMutex};
	players.push_back(connection);
	connection->setId(nextId++);
	connection->setGameLobby(this);
	connection->waitForMessages();
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
	return static_cast<Board::player_size_t>(players.size());
}

ConnectDisks * connectdisks::server::GameLobby::getGame() const noexcept
{
	return game.get();
}

void connectdisks::server::GameLobby::startLobby()
{
	lobbyIsOpen = true;
#if defined DEBUG || defined _DEBUG
	std::cout << "GameLobby " << this << " has started\n";
#endif
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
	return players.size() == maxPlayers;
}

bool connectdisks::server::GameLobby::allPlayersAreReady() const noexcept
{
	return numReady == players.size();
}
