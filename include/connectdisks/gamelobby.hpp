#pragma once

#include "connectdisks/board.hpp"
#include "connectdisks/connectdisks.hpp"
#include "connectdisks/connection.hpp"

#include "connectdisks/messaging.hpp"

#include <boost/asio.hpp>

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace connectdisks
{
	namespace server
	{
	// Runs a ConnectDisks game
		class GameLobby
		{
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

			// TODO - Finalize signal/slot for Connection ending and being removed from lobby
			void onDisconnect(std::shared_ptr<Connection> connection);

			// TODO - Finalize signal/slot interface
			void onReady(std::shared_ptr<Connection> connection);
		private:
			void startGame();
			void stopGame();
			void startLobby();

			bool isEmptyInternal() const noexcept;
			bool isFullInternal() const noexcept;
			bool allPlayersAreReady() const noexcept;

			std::atomic<bool> lobbyIsOpen;
			std::atomic<bool> isPlayingGame;
			std::atomic<bool> canAddPlayers;

			mutable std::mutex playersMutex;

			std::unique_ptr<ConnectDisks> game;
			Board::player_size_t maxPlayers;
			Board::player_size_t nextId;
			Board::player_size_t numReady;
			std::vector<std::shared_ptr<Connection>> players;
		};
	}
}
