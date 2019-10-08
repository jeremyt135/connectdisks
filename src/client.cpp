#include "connectdisks/client.hpp"

#include "connectdisks/connectdisks.hpp"
#include "connectdisks/server.hpp"

#include "type_utility.hpp"
#include "logging.hpp"

#include <boost/endian/arithmetic.hpp>
#include <boost/endian/conversion.hpp>

#include <array>
#include <iostream>
#include <thread>

using boost::asio::ip::address_v4;
using boost::asio::ip::tcp;

using namespace connectdisks::client;
using connectdisks::Board;
using connectdisks::ConnectDisks;

using typeutil::toUnderlyingType;
using typeutil::toScopedEnum;

connectdisks::client::Client::Client(boost::asio::io_service & ioService, std::string address, uint16_t port)
	:
	isPlaying{false},
	socket{ioService},
	playerId{0},
	game{nullptr}
{
	connectToServer(address, port);
}

connectdisks::client::Client::~Client()
{
}

void connectdisks::client::Client::connectToServer(std::string address, uint16_t port)
{
	try
	{
		socket.connect(tcp::endpoint{address_v4::from_string(address), port});
	}
	catch (std::exception& error)
	{
		printDebug("Client::connectToServer: error: ", error.what(), "\n");
		throw;
	}

	waitForMessages();
}

void connectdisks::client::Client::waitForMessages()
{
	printDebug("Client waiting for messages\n");
	// read message from server
	std::shared_ptr<server::Message> message{new server::Message{}};
	boost::asio::async_read(socket,
		boost::asio::buffer(message.get(), sizeof(server::Message)),
		std::bind(
			&Client::handleRead,
			this,
			message,
			std::placeholders::_1,
			std::placeholders::_2
		));
}

void connectdisks::client::Client::sendReady()
{
	// tell server we're ready to play
	std::shared_ptr<client::Message> message{new client::Message{}};
	message->response = toScopedEnum<client::Response>::cast(
		boost::endian::native_to_big(toUnderlyingType(client::Response::ready)));
	message->data[0] = boost::endian::native_to_big(playerId);
	boost::asio::async_write(socket,
		boost::asio::buffer(message.get(), sizeof(client::Message)),
		std::bind(
			&Client::handleWrite,
			this,
			message,
			std::placeholders::_1,
			std::placeholders::_2
		));
}

void connectdisks::client::Client::handleConnection(const boost::system::error_code & error)
{
	if (!error.failed())
	{
		printDebug("Client disconnected\n");
	}
	else
	{
		printDebug("Client::handleConnection: error: ", error.message(), "\n");
	}
}

void connectdisks::client::Client::handleRead(std::shared_ptr<server::Message> message, const boost::system::error_code & error, size_t len)
{
	if (!error.failed())
	{
		printDebug("Client received message\n");

		if (len == 0)
		{
			return;
		}

		const auto response{
			toScopedEnum<server::Response>::cast(
				boost::endian::big_to_native(toUnderlyingType(message->response))
			)
		};

		switch (response)
		{
		case server::Response::connected:
			onConnected(boost::endian::big_to_native(message->data[0]));
			break;
		case server::Response::gameStart:
		{
			const auto numPlayers = boost::endian::big_to_native(message->data[0]);
			const auto numCols = boost::endian::big_to_native(message->data[1]);
			const auto numRows = boost::endian::big_to_native(message->data[2]);
			const auto first = boost::endian::native_to_big(message->data[3]);
			printDebug(
				"Client is in a lobby that has started playing with "
				, static_cast<int>(numPlayers), " players\n");
			printDebug(
				"Board size is set to cols=", static_cast<int>(numCols), ", rows=",
				static_cast<int>(numRows), "\n");
			// ignore if already playing
			if (!isPlaying)
			{
				onGameStarted(numPlayers, first, numCols, numRows);
			}
		}
		break;
		case server::Response::gameEnd:
		{
			printDebug("Client is in a game that ended; returned to lobby\n");
			const auto winner{
				boost::endian::big_to_native(message->data[0])
			};
			onGameEnded(winner);
		}
		break;
		case server::Response::takeTurn:
			printDebug("Time for client to take turn\n");
			onTakeTurn();
			break;
		case server::Response::turnResult:
		{
			printDebug("Client received results of turn\n");
			const auto turnResult{
				toScopedEnum<ConnectDisks::TurnResult>::cast(
					boost::endian::big_to_native(message->data[0]))
			};
			const auto column{
				boost::endian::big_to_native(message->data[1])
			};
			onTurnResult(turnResult, column);
		}
		break;
		case server::Response::update:
		{
			printDebug("Client received an opponent's turn update\n");
			const auto player{
				boost::endian::big_to_native(message->data[0])
			};
			const auto column{
				boost::endian::big_to_native(message->data[1])
			};
			onUpdate(player, column);
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
			printDebug("Client::handleRead: connection ended \n");
			break;
		default:
			printDebug("Client::handleRead: unknown error occurred: ", error.message(), "\n");
			break;
		}

		onDisconnect();
	}
}

void connectdisks::client::Client::handleWrite(std::shared_ptr<client::Message> message, const boost::system::error_code & error, size_t len)
{
	if (!error.failed())
	{
		printDebug("Sent message to server\n");
	}
	else
	{
		printDebug("Client::handleWrite: error: ", error.message(), "\n");
	}
}

void connectdisks::client::Client::onConnected(Board::player_size_t id)
{
	playerId = id;
	printDebug("Client id set to: ", static_cast<int>(playerId), "\n");
	if (connectHandler)
	{
		connectHandler(id);
	}
	sendReady();
}

void connectdisks::client::Client::onDisconnect()
{
	stopPlaying();
	if (disconnectHandler)
	{
		disconnectHandler();
	}
}

void connectdisks::client::Client::onGameStarted(Board::player_size_t numPlayers, Board::player_size_t first, Board::board_size_t cols, Board::board_size_t rows)
{
	game.reset(new ConnectDisks{numPlayers, first, cols, rows});
	startPlaying();
	if (gameStartHandler)
	{
		gameStartHandler(numPlayers, first, cols, rows);
	}
}

void connectdisks::client::Client::onGameEnded(Board::player_size_t winner)
{
	stopPlaying();
	if (gameEndHandler)
	{
		gameEndHandler(winner);
	}
}

void connectdisks::client::Client::takeTurn(Board::board_size_t column)
{
	// send turn to server
	std::shared_ptr<client::Message> message{new client::Message{}};
	message->response = toScopedEnum<client::Response>::cast(
		boost::endian::native_to_big(toUnderlyingType(client::Response::turn)));
	message->data[0] = boost::endian::native_to_big(column);
	boost::asio::async_write(socket,
		boost::asio::buffer(message.get(), sizeof(client::Message)),
		std::bind(
			&Client::handleWrite,
			this,
			message,
			std::placeholders::_1,
			std::placeholders::_2
		));
}

void connectdisks::client::Client::onTakeTurn()
{
	if (turnHandler)
	{
		auto move = turnHandler();
		takeTurn(move);
	}
}

void connectdisks::client::Client::onTurnResult(ConnectDisks::TurnResult result, Board::board_size_t column)
{
	switch (result)
	{
	case ConnectDisks::TurnResult::success:
		game->takeTurn(playerId, column);
		if (turnResultHandler)
		{
			turnResultHandler(result, column);
		}
		break;
	default:
		printDebug("Unsuccessful turn, client sending new turn\n");
		if (turnResultHandler)
		{
			turnResultHandler(result, column);
		}
		onTakeTurn();
		break;
	}

}

void connectdisks::client::Client::onUpdate(Board::player_size_t player, Board::board_size_t col)
{
	// assume all turns sent to us from server are good
	game->takeTurn(player, col);
	if (updateHandler)
	{
		updateHandler(player, col);
	}
}

const ConnectDisks * connectdisks::client::Client::getGame() const noexcept
{
	return game.get();
}

void connectdisks::client::Client::startPlaying()
{
	printDebug("Client is playing\n");
	isPlaying = true;
}

void connectdisks::client::Client::stopPlaying()
{
	printDebug("Client has stopped playing\n");
	isPlaying = false;
}
