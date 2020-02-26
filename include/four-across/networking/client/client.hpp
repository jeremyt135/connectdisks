#pragma once

#include "four-across/game/board.hpp"
#include "four-across/game/game.hpp"

#include "four-across/networking/messaging.hpp"

#include "signals-helper.hpp"

#include <boost/asio.hpp>

#include <functional>
#include <memory>
#include <string>

namespace game
{
	namespace networking
	{
		namespace client
		{
			/*
				Allows user to play a FourAcross game online. Users should connect slots
				to all gameplay related signals to play the game.
			*/
			class Client
			{
				/*
					Signals for notifying users of Client that the game has changed state
					or for certain network actions.
				*/
				// User has connected
				ADD_SIGNAL(Connect, connected, void, uint8_t)
				// User connected but is in queue and will receive queue position updates
				ADD_SIGNAL(QueueUpdate, queueUpdated, void, uint64_t)
				// User has disconnected
				ADD_SIGNAL(Disconnect, disconnected, void)
			public:
				Client();
				Client(const Client&) = delete;
				virtual ~Client();

				Client& operator=(const Client&) = delete;

				// Connects to a FourAcross game server
				void connect(std::string address, uint16_t port);

				// Disconnects from the server. Useful to external threads that
				// need to manually disconnect while preserving the underlying context.
				void disconnect();

				// Disconnects from the server, closing the connection, and stops
				// any i/o operations. Useful to external threads needing
				// to forcibly stop the client while discarding the context.
				void stop();
			protected:
				// Sends the desired turn to the server.
				void sendTurn(uint8_t column);

				// Sets Client to ready, indicating that user is ready to play.
				void toggleReady();

				// Returns game instance for querying and displaying
				const FourAcross* getGame() const noexcept;
				
				/*
					Methods for notifying subscribers of changes in the game's connection status.
					Derived classes can override these to add additional work.
				*/
				// Notifies subscribers when the game client has connected to the server. 
				virtual void onConnect(uint8_t playerId);

				// Notifies subscribies when the game client has disconnected.
				virtual void onDisconnect();

				// Notifies subscribies when the game client has disconnected.
				virtual void onQueueUpdate(uint64_t queuePosition);

				/*
					Methods for handling changes in game state received from the server.
				*/
				// Handles the game starting with the number of players, the first player, and the board dimensions.
				virtual void onGameStart(uint8_t numPlayers, uint8_t firstPlayer, uint8_t cols, uint8_t rows) = 0;

				// Handles the end of the game. Derived classes can prompt for rematch and call toggleReady or immediately quit.
				virtual void onGameEnd(uint8_t winner) = 0;

				// Handles an update to the game state that did not originate from this client ( i.e. opponent turns).
				virtual void onGameUpdate(uint8_t player, uint8_t col) = 0;

				// Handles the validation result of the turn previously sent to the server.
				virtual void onTurnResult(FourAcross::TurnResult result, uint8_t column) = 0;

				// Handles a request to get the user's turn. Derived classes should obtain a value and call sendTurn with it.
				virtual void handleTurnRequest() = 0;

			private:
				void startGame(uint8_t numPlayers, uint8_t first, uint8_t cols, uint8_t rows);
				void stopGame(uint8_t winner);

				void setPlayerId(uint8_t id);

				void checkTurnResult(FourAcross::TurnResult result, uint8_t col);
				void takeOpponentTurn(uint8_t player, uint8_t col);

				void handleConnect(const boost::system::error_code& error);
				void handleRead(std::shared_ptr<Message> message, const boost::system::error_code& error, size_t len);
				void handleWrite(std::shared_ptr<Message> message, const boost::system::error_code& error, size_t len);

				void stopContext();

				void waitForMessages();

				void sendMessage(Message* message);
				void sendPong();

				std::unique_ptr<boost::asio::ip::tcp::socket> socket;
				boost::asio::io_context ioContext;

				uint8_t playerId;
				bool isConnected;

				std::unique_ptr<FourAcross> game;
			};
		}
	}
}