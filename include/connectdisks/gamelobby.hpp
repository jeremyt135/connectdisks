#pragma once

#include "connectdisks/board.hpp"
#include "connectdisks/connectdisks.hpp"
#include "connectdisks/connection.hpp"

#include "connectdisks/messaging.hpp"

#include "signals_helper.hpp"

#include <boost/asio.hpp>

#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace connectdisks
{
	namespace server
	{
		/* 
			Runs a ConnectDisks game. Clients do not connect to lobbies directly, rather the Server assigns
			them to one.
		*/
		class GameLobby
		{
			ADD_SIGNAL(GameStart, gameStarted, void)
			ADD_SIGNAL(GameEnd, gameEnded, void, Board::player_size_t)
			ADD_SIGNAL(Turn, takeTurn, void, Board::player_size_t)
			ADD_SIGNAL(TurnResult, tookTurn, void, Board::player_size_t, Board::board_size_t, ConnectDisks::TurnResult)

		public:
			GameLobby(Board::player_size_t maxPlayers = ConnectDisks::minNumPlayers);

			GameLobby(const GameLobby&) = delete;

			~GameLobby();

			GameLobby& operator=(const GameLobby&) = delete;

			// Starts the lobby and waits for enough players to start a game
			void start();

			// Adds a player (client connection) to the game lobby
			void addPlayer(std::shared_ptr<Connection> connection);

			// Returns true if no players are connected
			bool isEmpty() const noexcept;

			// Returns true if the max number of players are connected
			bool isFull() const noexcept;

			Board::player_size_t getNumPlayers() const noexcept;

			ConnectDisks* getGame() const noexcept;

		private:
			// Handles a Connection taking their turn
			void onTakeTurn(std::shared_ptr<Connection> connection, Board::board_size_t column);
			// Handles a Connection disconnecting from the lobby
			void onDisconnect(std::shared_ptr<Connection> connection);
			// Handles a Connection sending a ready signal
			void onReady(std::shared_ptr<Connection> connection);

			// Starts the game, locking players list and notifying players
			void startGame();
			// Aborts the game without a winner
			void stopGame();
			// Starts the lobby, allowing players to be added
			void startLobby();
			// Handles any necessary end of game cleanup
			void onGameOver();

			bool allPlayersAreReady() const noexcept;

			Board::player_size_t getFirstAvailableId() const;

			bool lobbyIsOpen;
			bool isPlayingGame;
			bool canAddPlayers;

			std::unique_ptr<ConnectDisks> game;
			Board::player_size_t maxPlayers;
			Board::player_size_t numReady;
			Board::player_size_t numPlayers;
			std::vector<std::shared_ptr<Connection>> players;
		};
	}
}
