#pragma once

#include "four-across/game/board.hpp"
#include "four-across/game/game.hpp"

#include "four-across/networking/messaging.hpp"

#include "signals-helper.hpp"

#include <boost/asio.hpp>

#include <functional>
#include <future>
#include <memory>
#include <string>

namespace game
{
	namespace client
	{
		/*
			Allows user to play a FourAcross game online. Users must connect slots
			to all gameplay related signals to play the game.
		*/
		class Client
		{
			/*
				Optional signals - users can still play the game without these but it may be difficult
				to track the game progress and send correct moves.
			*/
			ADD_SIGNAL(Connect, connected, void, uint8_t)
			ADD_SIGNAL(Disconnect, disconnected, void)
			ADD_SIGNAL(GameStart, gameStarted, void, uint8_t, uint8_t, uint8_t, uint8_t)
			ADD_SIGNAL(GameEnd, gameEnded, void, uint8_t)
			ADD_SIGNAL(TurnResult, tookTurn, void, FourAcross::TurnResult, uint8_t)
			ADD_SIGNAL(GameUpdate, gameUpdated, void, uint8_t, uint8_t)

			/*
				Required signals - users must provide a handler, and Client will throw a
				std::runtime_exception if one isn't provided. (Signals with a non-void return
				will use the value returned by last handler)
			*/
			ADD_SIGNAL(Turn, takeTurn, uint8_t)
			ADD_SIGNAL(ReadyStatus, askIfReady, bool)
			ADD_SIGNAL(RematchStatus, askIfRematch, bool)
		public:

			Client(boost::asio::io_service& ioService);
			Client(const Client&) = delete;
			~Client();

			Client& operator=(const Client&) = delete;
				
			// Connects to a FourAcross game server
			void connectToServer(std::string address, uint16_t port);

			// Returns const pointer to game instance, user can't take turns using this pointer
			const FourAcross* getGame() const noexcept;
		private:
			// Waits for data to be available on socket
			void waitForMessages();

			// Sends a message to Server
			void sendMessage(std::shared_ptr<client::Message> message);

			// Sends message to server indicating Client is ready to play
			void sendReady();

			// Sends message to server indicating Client wants a rematch
			void sendRematch(bool shouldRematch);

			void sendTurn(uint8_t column);

			void startPlaying();
			void stopPlaying();

			void disconnect();

			void getReadyStatus();
			void getRematchStatus();

			void handleConnection(const boost::system::error_code& error);
			void handleRead(std::shared_ptr<server::Message> message, const boost::system::error_code& error, size_t len);
			void handleWrite(std::shared_ptr<client::Message> message, const boost::system::error_code& error, size_t len);

			void onConnected(uint8_t id);
			void onDisconnect();

			void onGameStarted(uint8_t numPlayers, uint8_t first, uint8_t cols, uint8_t rows);
			void onGameEnded(uint8_t winner);

			void onTakeTurn();
			void onTurnResult(FourAcross::TurnResult result, uint8_t col);
			void onUpdate(uint8_t player, uint8_t col);

			bool isPlaying;

			boost::asio::ip::tcp::socket socket;

			uint8_t playerId;
			std::unique_ptr<FourAcross> game;
		};
	}
}