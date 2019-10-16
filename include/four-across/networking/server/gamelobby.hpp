#pragma once

#include "four-across/game/game.hpp"
#include "four-across/networking/server/connection.hpp"

#include "four-across/networking/messaging.hpp"

#include "signals-helper.hpp"

#include <boost/asio.hpp>

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace game
{
	namespace networking
	{
		namespace server
		{
			/*
				Runs a FourAcross game. Clients do not connect to lobbies directly, rather the Server assigns
				them to one.
			*/
			class GameLobby
			{
				ADD_SIGNAL(GameStart, gameStarted, void)
				ADD_SIGNAL(GameEnd, gameEnded, void, uint8_t)
				ADD_SIGNAL(Turn, takeTurn, void, uint8_t)
				ADD_SIGNAL(TurnResult, tookTurn, void, uint8_t, uint8_t, FourAcross::TurnResult)

				ADD_SIGNAL(LobbyAvailable, lobbyAvailable, void, GameLobby*)
			public:
				GameLobby(uint8_t maxPlayers = FourAcross::minNumPlayers);

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

				uint8_t getNumPlayers() const noexcept;

				FourAcross* getGame() const noexcept;

			private:
				// Handles a Connection taking their turn
				void onTakeTurn(std::shared_ptr<Connection> connection, uint8_t column);
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

				uint8_t getFirstAvailableId() const;

				bool lobbyIsOpen;
				bool isPlayingGame;
				bool canAddPlayers;

				std::unique_ptr<FourAcross> game;
				uint8_t maxPlayers;
				uint8_t numReady;
				uint8_t numPlayers;
				std::vector<std::shared_ptr<Connection>> players;
			};
		}
	}
}