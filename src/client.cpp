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

using namespace connectdisks;

using typeutil::toUnderlyingType;
using typeutil::toScopedEnum;

connectdisks::Client::Client(boost::asio::io_service & ioService, std::string address, uint16_t port)
	: socket{ioService},
	playerId{0},
	game{nullptr}
{
	if (!connectToServer(address, port))
	{
		throw std::runtime_error("Client::Client: failed to connect to server");
	}
}

connectdisks::Client::~Client()
{
}

bool connectdisks::Client::connectToServer(std::string address, uint16_t port)
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
		return false;
	}

#if defined DEBUG || defined _DEBUG
	std::cerr << "Client " << this << " created socket \n";
#endif

	try
	{

		{
			// get the "connected" response from the server
			ServerMessage message;

			size_t len = boost::asio::read(socket, boost::asio::buffer(&message, sizeof(ServerMessage)));
		#if defined DEBUG || _DEBUG
			std::cerr << "Client " << this << "read " << len << "bytes \n";
		#endif
			if (len == 0)
			{
				return false;
			}
			const auto response{
					toScopedEnum<ServerResponse>::cast(
						boost::endian::big_to_native(toUnderlyingType(message.response))
					)
			};

		#if defined DEBUG || _DEBUG
			std::cout << "Client " << this << "received response " <<
				static_cast<int>(toUnderlyingType(response))
				<< "\n";
		#endif

			if (response == ServerResponse::connected)
			{
			#if defined DEBUG || _DEBUG
				std::cout << "Client " << this << "connected to server\n";
			#endif
			}
		}

		{
		#if defined DEBUG || defined _DEBUG
			std::cerr << "Client " << this << " trying to request id from server \n";
		#endif
			ClientMessage message;
			message.request = toScopedEnum<ClientRequest>::cast(
				boost::endian::native_to_big(toUnderlyingType(ClientRequest::getId))
			);
			boost::asio::write(socket, boost::asio::buffer(&message, sizeof(ClientMessage)));
		#if defined DEBUG || defined _DEBUG
			std::cerr << "Client " << this << " sent request to server server \n";
		#endif
		}

	#if defined DEBUG || _DEBUG
		std::cerr << "Client " << this << "trying to get response from server \n";
	#endif
		{
			ServerMessage message;

			size_t len = boost::asio::read(socket, boost::asio::buffer(&message, sizeof(ServerMessage)));
		#if defined DEBUG || _DEBUG
			std::cerr << "Client " << this << "read " << len << "bytes \n";
		#endif
			if (len == 0)
			{
				return false;
			}
			const auto response{
					toScopedEnum<ServerResponse>::cast(
						boost::endian::big_to_native(toUnderlyingType(message.response))
					)
			};

		#if defined DEBUG || _DEBUG
			std::cout << "Client " << this << "received response " <<
				static_cast<int>(toUnderlyingType(response))
				<< "\n";
		#endif

			if (response == ServerResponse::id)
			{
				playerId = message.data[0];
			#if defined DEBUG || _DEBUG
				std::cout << "Client " << this << "received id " <<
					static_cast<int>(boost::endian::big_to_native(message.data[0]))
					<< "\n";
			#endif
				return playerId;
			}
		}

	}
	catch (std::exception& e)
	{
	#if defined DEBUG || defined _DEBUG
		std::cerr << "Client " << this << "::connectToServer: couldn't read id: " << e.what() << "\n";
	#endif
	}


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
