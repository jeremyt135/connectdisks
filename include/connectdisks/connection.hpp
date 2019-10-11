#pragma once

#include "connectdisks/board.hpp"
#include "connectdisks/connectdisks.hpp"

#include "connectdisks/messaging.hpp"

#include "signals_helper.hpp"

#include <boost/asio.hpp>

#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace connectdisks
{
	namespace server
	{
		class GameLobby;

		/* 
			Maintains connection from clients and handles socket I/O.
		*/
		class Connection : public std::enable_shared_from_this<Connection>
		{
			ADD_SIGNAL(Disconnect, disconnected, void, std::shared_ptr<Connection>)
			ADD_SIGNAL(Ready, readied, void, std::shared_ptr<Connection>)
			ADD_SIGNAL(Turn, tookTurn, void, std::shared_ptr<Connection>, Board::board_size_t)
			
		public:
			static std::shared_ptr<Connection> create(boost::asio::io_service& ioService, GameLobby* lobby = nullptr);


			// Starts an async read from the socket
			void waitForMessages();

			// Sets the id that the player should have
			void setId(Board::player_size_t id);

			Board::player_size_t getId() const noexcept;

			// Sets the GameLobby that this is connected to
			void setGameLobby(GameLobby* lobby);

			boost::asio::ip::tcp::socket& getSocket();
		private:
			Connection(boost::asio::io_service& ioService, GameLobby* lobby = nullptr);

			void onTurn(Board::player_size_t playerId);
			void onGameStart();
			void onGameEnd(Board::player_size_t winner);
			void onUpdate(Board::player_size_t playerId, Board::board_size_t col, ConnectDisks::TurnResult result);

			// Send message to client
			void sendMessage(std::shared_ptr<server::Message> message);

			// Handles messages from the client on other end of connection
			void handleRead(std::shared_ptr<client::Message> message, const boost::system::error_code& error, size_t len);

			// Handles result of sending message to client
			void handleWrite(std::shared_ptr<server::Message> message, const  boost::system::error_code& error, size_t len);

			void handleDisconnect();
			void handleClientReady();
			void handleTurnResult(ConnectDisks::TurnResult result, Board::board_size_t column);

			GameLobby* lobby;

			boost::asio::ip::tcp::socket socket;
			Board::player_size_t id;
		};
	}
}