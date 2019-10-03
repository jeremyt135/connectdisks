#include "connectdisks/client.hpp"

#include "connectdisks/server.hpp"

#include "type_utility.hpp"

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
	catch (std::exception& e)
	{
	#if defined DEBUG || _DEBUG
		std::cerr << "Client::connectToServer: " << e.what() << "\n";
	#endif
		throw;
	}

	waitForMessages();
}

void connectdisks::client::Client::waitForMessages()
{
#if defined DEBUG || defined _DEBUG
	std::cerr << "Client " << this << " waiting for messages\n";
#endif
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
		boost::asio::buffer(message.get(), sizeof(server::Message)),
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
	#if defined DEBUG || defined _DEBUG
		std::cerr << "Client " << this << " connected \n";
	#endif
	}
	else
	{
	#if defined DEBUG || defined _DEBUG
		std::cerr << "Client " << this << " failed to connect\n";
	#endif
	}
}

void connectdisks::client::Client::handleRead(std::shared_ptr<server::Message> message, const boost::system::error_code & error, size_t len)
{
	if (!error.failed())
	{
	#if defined DEBUG || defined _DEBUG
		std::cerr << "Client " << this << " received message \n";
	#endif

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
			playerId = boost::endian::big_to_native(message->data[0]);
		#if defined DEBUG || defined _DEBUG
			std::cerr << "Client " << this << " id set to: " << static_cast<int>(playerId) << " \n";
		#endif
			sendReady();
			break;
		case server::Response::gameStart:
		{
			const auto numPlayers = boost::endian::big_to_native(message->data[0]);
			const auto numCols = boost::endian::big_to_native(message->data[1]);
			const auto numRows = boost::endian::big_to_native(message->data[2]);
		#if defined DEBUG || defined _DEBUG
			std::cerr << "Client " << this << " is in a lobby that has started playing with "
				<< static_cast<int>(numPlayers) << " players\n";
			std::cerr << "Board size is set to cols=" << static_cast<int>(numCols) << ", rows=" <<
				static_cast<int>(numRows) << "\n";
		#endif
			// ignore if already playing
			if (!isPlaying)
			{
				game.reset(new ConnectDisks{numPlayers, 1, numCols, numRows});
				startPlaying();
			}
		}
		break;
		case server::Response::gameEnd:
		#if defined DEBUG || defined _DEBUG
			std::cerr << "Client " << this << " is in a game that ended; returned to lobby\n";
		#endif
			stopPlaying();
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
		#if defined DEBUG || defined _DEBUG
			std::cerr << "Client " << this << "::handleRead: connection ended \n";
		#endif
			break;
		default:
		#if defined DEBUG || defined _DEBUG
			std::cerr << "Client " << this << "::handleRead: " << error.message() << "\n";
		#endif
			break;
		}
	}
}

void connectdisks::client::Client::handleWrite(std::shared_ptr<client::Message> message, const boost::system::error_code & error, size_t len)
{
	if (!error.failed())
	{
	#if defined DEBUG || defined _DEBUG
		std::cerr << "Sent message to server\n";
	#endif
	}
	else
	{
	#if defined DEBUG || defined _DEBUG
		std::cerr << "Client::handleWrite: " << error.message() << "\n";
	#endif
	}
}

bool connectdisks::client::Client::takeTurn(Board::board_size_t column)
{
	return false;
}

const ConnectDisks * connectdisks::client::Client::getGame() const noexcept
{
	return nullptr;
}

void connectdisks::client::Client::startPlaying()
{
#if defined DEBUG || defined _DEBUG
	std::cerr << "Client " << this << " is playing\n";
#endif
	isPlaying = true;
}

void connectdisks::client::Client::stopPlaying()
{
#if defined DEBUG || defined _DEBUG
	std::cerr << "Client " << this << " has stopped playing\n";
#endif
	isPlaying = false;
}

connectdisks::server::Message connectdisks::client::Client::sendTurnToServer(Board::board_size_t column)
{
	return server::Message{};
}

Board::player_size_t connectdisks::client::Client::getPlayerIdFromServer()
{
	return 0;
}
