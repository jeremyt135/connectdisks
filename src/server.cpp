#include "connectdisks/server.hpp"

#include "connectdisks/connection.hpp"
#include "connectdisks/gamelobby.hpp"

#include "connectdisks/client.hpp"

#include "type_utility.hpp"

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
#if defined DEBUG || defined _DEBUG
	std::cerr << "[DEBUG] Server waiting for connections\n";
#endif

	std::shared_ptr<Connection> connection{Connection::create(ioService)};

	acceptor.async_accept(
		connection->getSocket(),
		std::bind(
			&Server::handleConnection, this,
			connection, std::placeholders::_1));
}

void connectdisks::server::Server::handleConnection(std::shared_ptr<Connection> connection, const boost::system::error_code & error)
{
#if defined DEBUG || defined _DEBUG
	std::cerr << "[DEBUG] Server trying to accept connection\n";
#endif
	if (!error.failed())
	{
		std::lock_guard<std::mutex> lock{lobbiesMutex};
		const auto numLobbies = lobbies.size();
	#if defined DEBUG || defined _DEBUG
		std::cerr << "[DEBUG] Server accepted connection, there are " << numLobbies << " lobbies \n";
	#endif
		// assign connection to an existing lobby if one exists
		if (numLobbies != 0)
		{
		#if defined DEBUG || defined _DEBUG
			std::cerr << "[DEBUG] Lobbies exist already\n";
		#endif
			auto lobby = std::find_if(
				lobbies.begin(),
				lobbies.end(),
				[](std::unique_ptr<GameLobby>& gameLobby){
					return !gameLobby->isFull();
				}
			);
			if (lobby != lobbies.end())
			{
			#if defined DEBUG || defined _DEBUG
				std::cout << "[DEBUG] Adding player to lobby " << std::distance(lobbies.begin(), lobby) << "\n";
			#endif
				auto gameLobby = lobby->get();
				gameLobby->addPlayer(connection);
			}
			else // all current lobbies are full
			{
				// only make new lobby if not at cap
				if (numLobbies < maxLobbies)
				{
				#if defined DEBUG || defined _DEBUG
					std::cout << "[DEBUG] Making new lobby and adding player\n";
				#endif
					// make a new lobby
					lobbies.emplace_back(new GameLobby{});
					auto& gameLobby = lobbies.back();
					gameLobby->addPlayer(connection);
					gameLobby->start();
				}
				else
				{
				#if defined DEBUG || defined _DEBUG
					std::cout << "[DEBUG] Server at lobby cap, player not added to lobby\n";
				#endif
				}
			}
		}
		else
		{
			// only make new lobby if not at cap
			if (numLobbies < maxLobbies)
			{
			#if defined DEBUG || defined _DEBUG
				std::cout << "[DEBUG] Making new lobby and adding player\n";
			#endif
				// make a new lobby
				lobbies.emplace_back(new GameLobby{});
				auto& gameLobby = lobbies.back();
				gameLobby->addPlayer(connection);
				gameLobby->start();
			}
			else
			{
			#if defined DEBUG || defined _DEBUG
				std::cout << "[DEBUG] Server at lobby cap, player not added to lobby\n";
			#endif
			}
		}
	}
	else
	{
	#if defined DEBUG || defined _DEBUG
		std::cerr << "[DEBUG] Server::acceptConnection: " << error.message() << "\n";
	#endif
		return;
	}

#if defined DEBUG || defined _DEBUG
	std::cout << "[DEBUG] Added player\n";
#endif

	waitForConnections();
}