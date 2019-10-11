#include "connectdisks/gamelobby.hpp"

#include "type_utility.hpp"

#include "logging.hpp"

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
	canAddPlayers{false},
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

		canAddPlayers = true;

		print("GameLobby[", this, "]: player disconnected; remaining: ", static_cast<int>(numPlayers), "\n");

		if (isPlayingGame)
		{
			stopGame();
		}

		// should close lobby entirely and destroy it in Server?
		if (isEmpty())
		{
			game.reset(); // for now ensure game is reset
		}
	}

}

void connectdisks::server::GameLobby::onReady(std::shared_ptr<Connection> connection)
{
	// find the connection in this lobby that has same id as the one that just readied
	auto player = std::find_if(players.begin(), players.end(),
		[connection](std::shared_ptr<Connection> con){
			return con->getId() == connection->getId();
		});
	if (player != players.end())
	{

		++numReady;
		if (allPlayersAreReady() && isFull())
		{
			game.reset(new ConnectDisks{maxPlayers}); // use default that first player is id 1

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

	try
	{
		const auto result = game->takeTurn(connection->getId(), column);
		switch (result)
		{
		case ConnectDisks::TurnResult::success:
		{
			// update all players appropriately
			//std::for_each(players.begin(), players.end(),
			//	[connection, column, result, this](std::shared_ptr<Connection> otherConnection){
			//		if (otherConnection != nullptr)
			//		{
			//			// tell other players that there was a move
			//			/*if (connection->getId() != otherConnection->getId())
			//			{
			//				otherConnection->onUpdate(connection->getId(), column);
			//			}*/
			//			//// tell all players that the game ended if there was a winner
			//			//if (game->hasWinner())
			//			//{
			//			//	otherConnection->onGameEnd(game->getWinner());
			//			//}
			//			//// tell all players that the game is over without a winner if board is full
			//			//else if (game->boardFull())
			//			//{
			//			//	otherConnection->onGameEnd(0);
			//			//}
			//			// tell the next player it's their turn
			//			/*else if (otherConnection->getId() == game->getCurrentPlayer())
			//			{
			//				otherConnection->onTurn();
			//			}*/
			//		}
			//	});

			// notify other players that there was a move
			tookTurn(connection->getId(), column);

			if (game->hasWinner() || game->boardFull())
			{
				onGameOver();
			}
			else
			{
				takeTurn(game->getCurrentPlayer());
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
	isPlayingGame = true;
	canAddPlayers = false;

	// tell the first player it's their turn
	/*for (auto& player : players)
	{
		if (player)
		{
			player->onGameStart();
		}
	}*/
	gameStarted();
	takeTurn(game->getCurrentPlayer());
	//for (auto& player : players)
	//{
	//	if (player)
	//	{
	//		player->onGameStart();
	//		// tell the first player it's their turn
	//		if (player->getId() == game->getCurrentPlayer())
	//		{
	//			player->onTurn();
	//		}
	//	}
	//}
}

void connectdisks::server::GameLobby::stopGame()
{
	// stop playing if lost a player
	print("GameLobby [", this, "]: is stopping game\n");
	gameEnded(game->noWinner);
	/*for (auto& player : players)
	{
		if (player)
		{
			player->onGameEnd(0);
		}
	}*/
}

void connectdisks::server::GameLobby::addPlayer(std::shared_ptr<Connection> connection)
{
	if (!canAddPlayers)
	{
		return;
	}

	const auto id = getFirstAvailableId();

	players[id] = connection;

	players[id]->setId(id + 1);
	players[id]->setGameLobby(this);
	players[id]->waitForMessages();

	++numPlayers;
}

bool connectdisks::server::GameLobby::isEmpty() const noexcept
{
	return players.empty();
}

bool connectdisks::server::GameLobby::isFull() const noexcept
{
	return numPlayers == maxPlayers;
}

Board::player_size_t connectdisks::server::GameLobby::getNumPlayers() const noexcept
{
	return static_cast<Board::player_size_t>(numPlayers);
}

ConnectDisks * connectdisks::server::GameLobby::getGame() const noexcept
{
	return game.get();
}

void connectdisks::server::GameLobby::startLobby()
{
	lobbyIsOpen = true;
	canAddPlayers = true;
	print("GameLobby [", this, "]: has started\n");
}

void connectdisks::server::GameLobby::onGameOver()
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
	game.reset();
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

