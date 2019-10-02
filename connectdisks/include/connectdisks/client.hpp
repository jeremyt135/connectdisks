#pragma once

#include "connectdisks/board.hpp"
#include "connectdisks/connectdisks.hpp"

#include <memory>
#include <string>

#include <boost/asio.hpp>

namespace connectdisks
{
	struct ServerMessage;

	class Client
	{
	public:
		enum class Request : uint8_t;

		Client(boost::asio::io_service& context);
		Client(const Client&) = delete;
		~Client();

		Client& operator=(const Client&) = delete;

		// Connects to a ConnectDisks game server
		bool connectToServer(std::string address, uint16_t port);

		// Attempts to take a turn in the given column
		bool takeTurn(Board::board_size_t column);

		const ConnectDisks* getGame() const noexcept;
	private:
		ServerMessage sendTurnToServer(Board::board_size_t column);
		Board::player_size_t getPlayerIdFromServer();

		boost::asio::ip::tcp::socket socket;

		Board::player_size_t playerId;
		std::unique_ptr<ConnectDisks> game;
	};

	enum class Client::Request : uint8_t
	{
		getId, takeTurn
	};

	struct ClientMessage
	{
		Client::Request request;
		std::array<uint8_t, 3> data;
	};
}
