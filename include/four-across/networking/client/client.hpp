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
				// User has successfully connected & should ready when they want to play
				ADD_SIGNAL(Connect, connected, void, uint8_t)
				// User has successfully disconnected
				ADD_SIGNAL(Disconnect, disconnected, void)
				// Game has started
				ADD_SIGNAL(GameStart, gameStarted, void, uint8_t, uint8_t, uint8_t, uint8_t)
				// Game has ended
				ADD_SIGNAL(GameEnd, gameEnded, void, uint8_t)
				// Client received result of previous turn
				ADD_SIGNAL(TurnResult, turnResult, void, FourAcross::TurnResult, uint8_t)
				// Client received result of opponent turn
				ADD_SIGNAL(GameUpdate, gameUpdated, void, uint8_t, uint8_t)
				// User should take their turn
				ADD_SIGNAL(TurnRequest, requestTurn, void)

			public:

				Client(boost::asio::io_service& ioService);
				Client(const Client&) = delete;
				~Client();

				Client& operator=(const Client&) = delete;

				// Connects to a FourAcross game server
				void connectToServer(std::string address, uint16_t port);

				// Sets Client to ready, indicating that user is ready to play. Users
				// should call this function only after connecting to a server successfully.
				void toggleReady();

				// Disconnects from game server.
				void disconnect();

				// Attempts to take the user's desired turn. Users should only call this 
				// function after their TurnRequest handler is called.
				void takeTurn(uint8_t column);

				// Returns game instance for querying and displaying
				const FourAcross* getGame() const noexcept;
			private:
				// Waits for data to be available on socket
				void waitForMessages();

				// Socket I/O callbacks
				void onConnect(const boost::system::error_code& error);
				void onReadSocket(std::shared_ptr<Message> message, const boost::system::error_code& error, size_t len);
				void onWriteSocket(std::shared_ptr<Message> message, const boost::system::error_code& error, size_t len);

				void sendMessage(Message* message);

				void handleDisconnect();

				void startGame(uint8_t numPlayers, uint8_t first, uint8_t cols, uint8_t rows);
				void stopGame(uint8_t winner);

				void setPlayerId(uint8_t id);

				void checkTurnResult(FourAcross::TurnResult result, uint8_t col);
				void takeOpponentTurn(uint8_t player, uint8_t col);

				boost::asio::ip::tcp::socket socket;

				uint8_t playerId;
				std::unique_ptr<FourAcross> game;
			};
		}
	}
}