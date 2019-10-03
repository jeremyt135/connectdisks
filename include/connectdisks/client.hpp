#pragma once

#include "connectdisks/board.hpp"
#include "connectdisks/connectdisks.hpp"

#include "connectdisks/messaging.hpp"

#include <boost/asio.hpp>

#include <memory>
#include <string>

namespace connectdisks
{

	// TODO - MAKE ASYNC
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

		// Attempts to take a turn in the given column
		bool takeTurn(Board::board_size_t column);

		const ConnectDisks* getGame() const noexcept;
	private:
		// Connects to a ConnectDisks game server
		void connectToServer(std::string address, uint16_t port);

		void waitForMessages();
		void sendReady();

		void handleConnection(const boost::system::error_code& error);
		void handleRead(std::shared_ptr<ServerMessage> message, const boost::system::error_code& error, size_t len);
		void handleWrite(std::shared_ptr<ClientMessage> message, const boost::system::error_code& error, size_t len);

		// TODO - create hooks into game loop to allow user of Client to take turns, etc
		void startPlaying();
		void stopPlaying();

		ServerMessage sendTurnToServer(Board::board_size_t column);
		Board::player_size_t getPlayerIdFromServer();

		std::atomic<bool> isPlaying;

		boost::asio::ip::tcp::socket socket;

		Board::player_size_t playerId;
		std::unique_ptr<ConnectDisks> game;
	};
}
