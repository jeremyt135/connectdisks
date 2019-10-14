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
		if (numReady > 0)
		{
			--numReady;
		}

		canAddPlayers = true;

		print("GameLobby[", this, "]: player disconnected; remaining: ", static_cast<int>(numPlayers), "\n");

		if (isPlayingGame)
		{
			stopGame();
		}
	}

}

void connectdisks::server::GameLobby::onReady(std::shared_ptr<Connection> connection)
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
			game.reset(new ConnectDisks{maxPlayers}); // use default that first player is id 1

			// start if all players are ready
			startGame();
		}
	}
}

void connectdisks::server::GameLobby::onTakeTurn(std::shared_ptr<Connection> connection, Board::board_size_t column)
{
	if (connection == nullptr || !isPlayingGame)
	{
		tookTurn(connection->getId(), column, ConnectDisks::TurnResult::error);
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
		else if (result == ConnectDisks::TurnResult::success)
		{
			// if turn was successful, tell next player to take turn
			takeTurn(game->getCurrentPlayer());
		}
	}
	catch (std::exception& error)
	{
		printDebug("GameLobby[", this, "]::onTakeTurn: error taking turn: ", error.what(), "\n");
		tookTurn(connection->getId(), column, ConnectDisks::TurnResult::error);
	}
}

void connectdisks::server::GameLobby::startGame()
{
	isPlayingGame = true;
	canAddPlayers = false;

	gameStarted();
	takeTurn(game->getCurrentPlayer());
}

void connectdisks::server::GameLobby::stopGame()
{
	// stop playing if lost a player
	print("GameLobby [", this, "]: is stopping game\n");
	numReady = 0;
	isPlayingGame = false;
	game.reset();
	gameEnded(game->noWinner);
}

void connectdisks::server::GameLobby::addPlayer(std::shared_ptr<Connection> connection)
{
	using std::placeholders::_1;
	using std::placeholders::_2;
	using std::bind;

	if (!canAddPlayers)
	{
		return;
	}

	const auto id = getFirstAvailableId();

	players[id] = connection;

	players[id]->setId(id + 1);
	players[id]->setGameLobby(this);
	players[id]->waitForMessages();
	connection->addDisconnectHandler(bind(&GameLobby::onDisconnect, this, _1));
	connection->addReadyHandler(bind(&GameLobby::onReady, this, _1));
	connection->addTurnHandler(bind(&GameLobby::onTakeTurn, this, _1, _2));
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
	numReady = 0;
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

