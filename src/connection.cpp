#include "connectdisks/connection.hpp"
#include "connectdisks/gamelobby.hpp"

#include "type_utility.hpp"
#include "logging.hpp"

#include <boost/endian/arithmetic.hpp>
#include <boost/endian/conversion.hpp>

#include <algorithm>
#include <iostream>
#include <functional>

using boost::asio::ip::address_v4;
using boost::asio::ip::tcp;

using namespace connectdisks::server;
using connectdisks::Board;
using connectdisks::ConnectDisks;

using typeutil::toUnderlyingType;
using typeutil::toScopedEnum;

std::shared_ptr<Connection> connectdisks::server::Connection::create(boost::asio::io_service & ioService, GameLobby* lobby)
{
	return std::shared_ptr<Connection>{new Connection{ioService, lobby}};
}

void connectdisks::server::Connection::onGameStart()
{
	// send id to client
	std::shared_ptr<server::Message> response{new server::Message{}};
	response->response = toScopedEnum<server::Response>::cast(
		boost::endian::native_to_big(toUnderlyingType(server::Response::gameStart)));

	// send the number of players
	response->data[0] = boost::endian::native_to_big(lobby->getNumPlayers());

	// send the board dimensions
	auto* game = lobby->getGame();

	response->data[1] = boost::endian::native_to_big(game->getNumColumns());
	response->data[2] = boost::endian::native_to_big(game->getNumRows());
	response->data[3] = boost::endian::native_to_big(game->getCurrentPlayer());

	sendMessage(response);
}

void connectdisks::server::Connection::onGameEnd(Board::player_size_t player)
{
	// tell client game has ended and which player won
	std::shared_ptr<server::Message> response{new server::Message{}};
	response->response = toScopedEnum<server::Response>::cast(
		boost::endian::native_to_big(toUnderlyingType(server::Response::gameEnd)));
	response->data[0] = boost::endian::native_to_big(player);
	sendMessage(response);
}

void connectdisks::server::Connection::onTurn()
{
	// tell client it's their turn
	std::shared_ptr<server::Message> response{new server::Message{}};
	response->response = toScopedEnum<server::Response>::cast(
		boost::endian::native_to_big(toUnderlyingType(server::Response::takeTurn)));
	sendMessage(response);
}

void connectdisks::server::Connection::onUpdate(Board::player_size_t player, Board::board_size_t col)
{
	// tell client that a successful turn was taken by anotehr player
	std::shared_ptr<server::Message> response{new server::Message{}};
	response->response = toScopedEnum<server::Response>::cast(
		boost::endian::native_to_big(toUnderlyingType(server::Response::update)));
	response->data[0] = boost::endian::native_to_big(player);
	response->data[1] = boost::endian::native_to_big(col);
	sendMessage(response);
}

void connectdisks::server::Connection::waitForMessages()
{
	printDebug("Connection waiting to read message\n");
	// read a message from the client, handle in handleRead
	std::shared_ptr<client::Message> message{new client::Message{}};
	boost::asio::async_read(socket,
		boost::asio::buffer(message.get(), sizeof(client::Message)),
		std::bind(
			&Connection::handleRead,
			this,
			message,
			std::placeholders::_1,
			std::placeholders::_2
		));
}

void connectdisks::server::Connection::setId(Board::player_size_t id)
{
	if (this->id == 0)
	{
		this->id = id;

		// send id to client
		std::shared_ptr<server::Message> response{new server::Message{}};
		response->response = toScopedEnum<server::Response>::cast(
			boost::endian::native_to_big(toUnderlyingType(server::Response::connected)));
		response->data[0] = boost::endian::native_to_big(id);
		sendMessage(response);
	}
}

connectdisks::Board::player_size_t connectdisks::server::Connection::getId() const noexcept
{
	return id;
}

void connectdisks::server::Connection::setGameLobby(GameLobby * lobby)
{
	this->lobby = lobby;
}

boost::asio::ip::tcp::socket& connectdisks::server::Connection::getSocket()
{
	return socket;
}

connectdisks::server::Connection::Connection(boost::asio::io_service & ioService, GameLobby* lobby) :
	lobby{lobby},
	socket{ioService},
	id{0}
{
}

void connectdisks::server::Connection::sendMessage(std::shared_ptr<server::Message> message)
{
	boost::asio::async_write(socket,
		boost::asio::buffer(message.get(), sizeof(server::Message)),
		std::bind(
			&Connection::handleWrite,
			this,
			message,
			std::placeholders::_1,
			std::placeholders::_2
		));
}

void connectdisks::server::Connection::handleRead(std::shared_ptr<connectdisks::client::Message> message, const boost::system::error_code & error, size_t len)
{
	printDebug("Connection trying to read message\n");
	if (!error.failed())
	{

		if (len == 0)
		{
			printDebug("Connection received 0 length message\n");
			return;
		}

		const auto request{
			toScopedEnum<client::Response>::cast(
				boost::endian::big_to_native(toUnderlyingType(message->response))
			)
		};
		switch (request)
		{
		case client::Response::ready:
			printDebug("Connection: client is ready\n");
			handleClientReady();
			break;
		case client::Response::turn:
		{
			const auto column{
				boost::endian::big_to_native(message->data[0])
			};
			printDebug("Connection: client wants to move in column: ", static_cast<int>(column), "\n");
			const auto result = lobby->onTakeTurn(shared_from_this(), column);
			handleTurnResult(result, column);
		}
			break;
		default:
			break;
		}

		waitForMessages();
	}
	else
	{
		switch (error.value())
		{
		case boost::asio::error::eof:
		case boost::asio::error::connection_aborted:
		case boost::asio::error::connection_reset:
			printDebug("Connection::handleRead: error: client disconnected\n");
			handleDisconnect();
			break;
		default:
			printDebug("Connection::handleRead: error reading data: ", error.message(), "\n");
			break;
		}
	}

}

void connectdisks::server::Connection::handleWrite(std::shared_ptr<server::Message> message, const boost::system::error_code & error, size_t len)
{
	if (!error.failed())
	{
		printDebug("Sent message to client\n");
	}
	else
	{
		printDebug("Connection::handleWrite: error writing data: ", error.message(), "\n");
	}
}

void connectdisks::server::Connection::handleDisconnect()
{
	lobby->onDisconnect(shared_from_this());
}

void connectdisks::server::Connection::handleClientReady()
{
	lobby->onReady(shared_from_this());
}

void connectdisks::server::Connection::handleTurnResult(ConnectDisks::TurnResult result, Board::board_size_t column)
{
	// send turn result to client
	std::shared_ptr<server::Message> response{new server::Message{}};
	response->response = toScopedEnum<server::Response>::cast(
		boost::endian::native_to_big(toUnderlyingType(server::Response::turnResult)));
	response->data[0] = boost::endian::native_to_big(toUnderlyingType(result));
	response->data[1] = boost::endian::native_to_big(column);
	sendMessage(response);
}
