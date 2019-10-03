#include "connectdisks/server.hpp"

#include "connectdisks/client.hpp"

#include "type_utility.hpp"

#include <boost/endian/arithmetic.hpp>
#include <boost/endian/conversion.hpp>

#include <algorithm>
#include <iostream>
#include <functional>

using boost::asio::ip::address_v4;
using boost::asio::ip::tcp;

using namespace connectdisks;

using typeutil::toUnderlyingType;
using typeutil::toScopedEnum;

connectdisks::Server::Server(
	boost::asio::io_service & ioService, 
	std::string address, uint16_t port
	) :
	ioService{ioService},
	acceptor{ioService, tcp::endpoint{address_v4::from_string(address), port}}
{
	waitForConnections();
}

connectdisks::Server::~Server()
{
}

void connectdisks::Server::waitForConnections()
{
#if defined DEBUG || defined _DEBUG
	std::cerr << "Server waiting for connections\n";
#endif
	std::shared_ptr<Connection> connection{Connection::create(ioService)};

	acceptor.async_accept(
		connection->getSocket(),
		std::bind(
			&Server::acceptConnection, this, 
			connection, std::placeholders::_1));
}

void connectdisks::Server::acceptConnection(std::shared_ptr<Connection> connection, const boost::system::error_code & error)
{
#if defined DEBUG || defined _DEBUG
	std::cerr << "Server trying to accept connection\n";
#endif
	if (!error.failed())
	{
	#if defined DEBUG || defined _DEBUG
		std::cerr << "Server accepted connection \n";
	#endif
		// assign connection to an existing lobby if one exists
		if (!lobbies.empty())
		{
		#if defined DEBUG || defined _DEBUG
			std::cerr << "Lobbies exist already\n";
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
				std::cout << "Adding player to lobby " << std::distance(lobbies.begin(), lobby) << "\n";
			#endif
				auto gameLobby = lobby->get();
				gameLobby->addPlayer(connection);
			}
		}
		else
		{
		#if defined DEBUG || defined _DEBUG
			std::cout << "Making new lobby and adding player\n";
		#endif
			// make a new lobby
			lobbies.emplace_back(new GameLobby{});
			auto& gameLobby = lobbies.back();
			gameLobby->addPlayer(connection);
			gameLobby->start();
		}
	}
	else{
	#if defined DEBUG || defined _DEBUG
		std::cerr << "Server::acceptConnection: " << error.message() << "\n";
	#endif
	}

	waitForConnections();
}

std::shared_ptr<Server::Connection> connectdisks::Server::Connection::create(boost::asio::io_service & ioService)
{
	return std::shared_ptr<Connection>{new Connection{ioService}};
}

void connectdisks::Server::Connection::waitForMessages()
{
#if defined DEBUG || defined _DEBUG
	std::cerr << "Connection waiting for messages\n";
#endif
	
	auto dataAvailableSignal = std::async([this](){ while (!socket.available()) {}});
	dataAvailableSignal.wait();

	// read a message from the client, handle in readMessage
	std::shared_ptr<ClientMessage> message{new ClientMessage{}};
	boost::asio::async_read(socket,
		boost::asio::buffer(message.get(), sizeof(ClientMessage)),
		std::bind(
			&Connection::readMessage, 
			this,
			message, 
			std::placeholders::_1,
			std::placeholders::_2
			));
}

void connectdisks::Server::Connection::setId(Board::player_size_t id)
{
	if (this->id == 0)
	{
		this->id = id;
	}
}

boost::asio::ip::tcp::socket& connectdisks::Server::Connection::getSocket()
{
	return socket;
}

connectdisks::Server::Connection::Connection(boost::asio::io_service & ioService) :
	socket{ioService},
	id{0}
{
}

void connectdisks::Server::Connection::readMessage(std::shared_ptr<connectdisks::ClientMessage> message, const boost::system::error_code & error, size_t len)
{
#if defined DEBUG || defined _DEBUG
	std::cerr << "Connection trying to read message\n";
#endif
	if (!error.failed())
	{
		if (len == 0)
		{
		#if defined DEBUG || defined _DEBUG
			std::cerr << "Connection received 0 length message\n";
		#endif
			return;
		}

		const auto request{
			toScopedEnum<ClientRequest>::cast(
				boost::endian::big_to_native(toUnderlyingType(message->request))
			)
		};
		if (request == ClientRequest::getId)
		{
		#if defined DEBUG || defined _DEBUG
			std::cout << "Client asked for id\n";
		#endif
			std::shared_ptr<ServerMessage> response{new ServerMessage{}};
			response->response = toScopedEnum<ServerResponse>::cast(boost::endian::native_to_big(toUnderlyingType(ServerResponse::id)));
			response->data[0] = boost::endian::native_to_big(id);
			boost::asio::async_write(socket,
				boost::asio::buffer(response.get(), sizeof(ServerMessage)),
				std::bind(
					&Connection::sendMessage,
					this,
					response,
					std::placeholders::_1,
					std::placeholders::_2
				));
		}
	}
	else
	{
		if (error != boost::asio::error::eof)
		{
		#if defined DEBUG || defined _DEBUG
			std::cerr << "Connection::readMessage: " << error.message() << "\n";
		#endif
		}
	}

	waitForMessages();
}

void connectdisks::Server::Connection::sendMessage(std::shared_ptr<ServerMessage> message, const boost::system::error_code & error, size_t len)
{
	if (!error.failed())
	{
	#if defined DEBUG || defined _DEBUG
		std::cerr << "Sent message to client\n";
	#endif
	}
	else
	{
	#if defined DEBUG || defined _DEBUG
		std::cerr << "Connection::sendMessage: " << error.message() << "\n";
	#endif
	}
}

connectdisks::Server::GameLobby::GameLobby(Board::player_size_t maxPlayers) :
	shouldClose{true},
	canAddPlayers{true},
	maxPlayers{maxPlayers}
{
	players.reserve(maxPlayers);
}

connectdisks::Server::GameLobby::~GameLobby()
{
	shouldClose = true;
	lobby.wait();
}

void connectdisks::Server::GameLobby::start()
{
	shouldClose = false;
	lobby = std::async(std::launch::async, &GameLobby::startLobby, this);
}

void connectdisks::Server::GameLobby::startGame()
{
	canAddPlayers = false;
	startGameSignal.set_value();
}

void connectdisks::Server::GameLobby::addPlayer(std::shared_ptr<Connection> connection)
{
	std::lock_guard<std::mutex> lock(playersMutex);
	players.push_back(connection);
	connection->setId(static_cast<Board::player_size_t>(players.size()));
	connection->waitForMessages();
	if (isFullInternal())
	{
		// start if it's full, for now
		startGame();
	}
}

bool connectdisks::Server::GameLobby::isEmpty() const noexcept
{
	std::lock_guard<std::mutex> lock(playersMutex);
	return isEmptyInternal();
}

bool connectdisks::Server::GameLobby::isFull() const noexcept
{
	std::lock_guard<std::mutex> lock(playersMutex);
	return isFullInternal();
}

void connectdisks::Server::GameLobby::startLobby()
{
	startGameSignal.get_future().wait();

#if defined DEBUG || defined _DEBUG
	std::cout << "GameLobby " << this << " has started playing\n";
#endif

	// start playing game
	while (!shouldClose)
	{

	}
}

bool connectdisks::Server::GameLobby::isEmptyInternal() const noexcept
{
	return players.empty();
}

bool connectdisks::Server::GameLobby::isFullInternal() const noexcept
{
	return players.size() == maxPlayers;
}
