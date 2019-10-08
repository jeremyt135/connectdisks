#include "connectdisks/server.hpp"

#include "connectdisks/connection.hpp"
#include "connectdisks/gamelobby.hpp"

#include "connectdisks/client.hpp"

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

using typeutil::toUnderlyingType;
using typeutil::toScopedEnum;

connectdisks::server::Server::Server(
	boost::asio::io_service & ioService,
	std::string address, uint16_t port
) :
	ioService{ioService},
	acceptor{ioService, tcp::endpoint{address_v4::from_string(address), port}}
{
	waitForConnections();
}

connectdisks::server::Server::~Server()
{
}

void connectdisks::server::Server::waitForConnections()
{
	printDebug("Server waiting for connections\n");

	std::shared_ptr<Connection> connection{Connection::create(ioService)};

	acceptor.async_accept(
		connection->getSocket(),
		std::bind(
			&Server::handleConnection, this,
			connection, std::placeholders::_1));
}

void connectdisks::server::Server::handleConnection(std::shared_ptr<Connection> connection, const boost::system::error_code & error)
{
	printDebug("Server trying to accept connection\n");

	if (!error.failed())
	{
		std::lock_guard<std::mutex> lock{lobbiesMutex};
		const auto numLobbies = lobbies.size();
		printDebug("Server accepted connection, there are ", numLobbies, " lobbies \n");
		// assign connection to an existing lobby if one exists
		if (numLobbies != 0)
		{
			auto lobby = std::find_if(
				lobbies.begin(),
				lobbies.end(),
				[](std::unique_ptr<GameLobby>& gameLobby){
					return !gameLobby->isFull();
				}
			);
			if (lobby != lobbies.end())
			{
				printDebug("Adding player to lobby ", std::distance(lobbies.begin(), lobby), "\n");
				auto gameLobby = lobby->get();
				gameLobby->addPlayer(connection);
			}
			else // all current lobbies are full
			{
				// only make new lobby if not at cap
				if (numLobbies < maxLobbies)
				{
					printDebug("Making new lobby and adding player\n");
					// make a new lobby
					lobbies.emplace_back(new GameLobby{});
					auto& gameLobby = lobbies.back();
					gameLobby->addPlayer(connection);
					gameLobby->start();
				}
				else
				{
					printDebug("Server at lobby cap, not adding player\n");
				}
			}
		}
		else
		{
			// only make new lobby if not at cap
			if (numLobbies < maxLobbies)
			{
				printDebug("Making new lobby and adding player\n");
				// make a new lobby
				lobbies.emplace_back(new GameLobby{});
				auto& gameLobby = lobbies.back();
				gameLobby->addPlayer(connection);
				gameLobby->start();
			}
			else
			{
				printDebug("Server at lobby cap, not adding player\n");
			}
		}
	}
	else
	{
		printDebug("Server::handleConnection: error accepting connection: ", error.message(), "\n");
		return;
	}

	printDebug("Added player\n");

	waitForConnections();
}