#pragma once

#include "connectdisks/board.hpp"
#include "connectdisks/connectdisks.hpp"

#include "connectdisks/messaging.hpp"

#include <boost/asio.hpp>

#include <functional>
#include <memory>
#include <string>

namespace connectdisks
{
	namespace client
	{
		using ConnectHandler	= std::function<void(Board::player_size_t)>;
		using DisconnectHandler = std::function<void()>;

		using GameStartHandler	= std::function<void(Board::player_size_t, Board::player_size_t, Board::board_size_t, Board::board_size_t)>;
		using GameEndHandler	= std::function<void(Board::player_size_t)>;

		using TurnHandler		= std::function<Board::board_size_t()>;
		using TurnResultHandler	= std::function<void(ConnectDisks::TurnResult, Board::board_size_t)>;
		using UpdateHandler		= std::function<void(Board::player_size_t, Board::board_size_t)>;

		class Client
		{
		public:

			Client(
				boost::asio::io_service& ioService,
				std::string address,
				uint16_t port);
			Client(const Client&) = delete;
			~Client();

			Client& operator=(const Client&) = delete;
				
			ConnectHandler		connectHandler;
			DisconnectHandler	disconnectHandler;

			GameStartHandler	gameStartHandler;
			GameEndHandler		gameEndHandler;

			TurnHandler			turnHandler;
			TurnResultHandler	turnResultHandler;

			UpdateHandler		updateHandler;

			// Returns const pointer to game instance, user can't take turns using this pointer
			const ConnectDisks* getGame() const noexcept;
		private:
			// Connects to a ConnectDisks game server
			void connectToServer(std::string address, uint16_t port);

			void waitForMessages();
			void sendReady();

			void handleConnection(const boost::system::error_code& error);
			void handleRead(std::shared_ptr<server::Message> message, const boost::system::error_code& error, size_t len);
			void handleWrite(std::shared_ptr<client::Message> message, const boost::system::error_code& error, size_t len);

			void onConnected(Board::player_size_t id);
			void onDisconnect();

			void onGameStarted(Board::player_size_t numPlayers, Board::player_size_t first, Board::board_size_t cols, Board::board_size_t rows);
			void onGameEnded(Board::player_size_t winner);

			void takeTurn(Board::board_size_t column);
			void onTakeTurn();
			void onTurnResult(ConnectDisks::TurnResult result, Board::board_size_t col);
			void onUpdate(Board::player_size_t player, Board::board_size_t col);

			void startPlaying();
			void stopPlaying();

			std::atomic<bool> isPlaying;

			boost::asio::ip::tcp::socket socket;

			Board::player_size_t playerId;
			std::unique_ptr<ConnectDisks> game;
		};
	}
}
