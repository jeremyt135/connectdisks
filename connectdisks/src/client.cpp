#include "connectdisks/client.hpp"

#include "connectdisks/server.hpp"

#include <boost/endian/arithmetic.hpp>
#include <boost/endian/conversion.hpp>

#include <array>
#include <iostream>
#include <thread>

using namespace connectdisks;
using boost::asio::ip::tcp;

connectdisks::Client::Client(boost::asio::io_service & context) : 
	socket{context}, 
	playerId{0},
	game{nullptr}
{
}

connectdisks::Client::~Client()
{
}

bool connectdisks::Client::connectToServer(std::string address, uint16_t port)
{
	return playerId != 0;
}

bool connectdisks::Client::takeTurn(Board::board_size_t column)
{

	return false;
}

const ConnectDisks * connectdisks::Client::getGame() const noexcept
{
	return nullptr;
}

connectdisks::ServerMessage connectdisks::Client::sendTurnToServer(Board::board_size_t column)
{
	return ServerMessage{};
}

Board::player_size_t connectdisks::Client::getPlayerIdFromServer()
{
	return 0;
}
