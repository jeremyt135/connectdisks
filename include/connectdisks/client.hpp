#pragma once

#include "connectdisks/board.hpp"
#include "connectdisks/connectdisks.hpp"

#include "connectdisks/messaging.hpp"

#include "signals_helper.hpp"

#include <boost/asio.hpp>

#include <functional>
#include <memory>
#include <string>

namespace connectdisks
{
	namespace client
	{
		/*
			Allows user to play a ConnectDisks game online. Users must connect slots
			to all gameplay related signals to play the game.
		*/
		class Client
		{
			/*
				Optional signals - users can still play the game without these but it may be difficult
				to track the game progress and send correct moves.
			*/
			ADD_SIGNAL(Connect, connected, void, Board::player_size_t)
			ADD_SIGNAL(Disconnect, disconnected, void)
			ADD_SIGNAL(GameStart, gameStarted, void, Board::player_size_t, Board::player_size_t, Board::board_size_t, Board::board_size_t)
			ADD_SIGNAL(GameEnd, gameEnded, void, Board::player_size_t)
			ADD_SIGNAL(TurnResult, tookTurn, void, ConnectDisks::TurnResult, Board::board_size_t)
			ADD_SIGNAL(GameUpdate, gameUpdated, void, Board::player_size_t, Board::board_size_t)

			/*
				Required signals - users must provide a handler, and Client will throw a
				std::runtime_exception if one isn't provided.
			*/
			ADD_SIGNAL(Turn, takeTurn, Board::player_size_t) // The last handler will provide the return value
			
		public:

			Client(boost::asio::io_service& ioService);
			Client(const Client&) = delete;
			~Client();

			Client& operator=(const Client&) = delete;
				
			// Connects to a ConnectDisks game server
			void connectToServer(std::string address, uint16_t port);

			// Sends message to server indicating Client is ready to play
			void sendReady();

			// Returns const pointer to game instance, user can't take turns using this pointer
			const ConnectDisks* getGame() const noexcept;
		private:
			void waitForMessages();

			void handleConnection(const boost::system::error_code& error);
			void handleRead(std::shared_ptr<server::Message> message, const boost::system::error_code& error, size_t len);
			void handleWrite(std::shared_ptr<client::Message> message, const boost::system::error_code& error, size_t len);

			void onConnected(Board::player_size_t id);
			void onDisconnect();

			void onGameStarted(Board::player_size_t numPlayers, Board::player_size_t first, Board::board_size_t cols, Board::board_size_t rows);
			void onGameEnded(Board::player_size_t winner);

			void sendTurn(Board::board_size_t column);
			void onTakeTurn();
			void onTurnResult(ConnectDisks::TurnResult result, Board::board_size_t col);
			void onUpdate(Board::player_size_t player, Board::board_size_t col);

			void startPlaying();
			void stopPlaying();

			bool isPlaying;

			boost::asio::ip::tcp::socket socket;

			Board::player_size_t playerId;
			std::unique_ptr<ConnectDisks> game;
		};
	}
}
