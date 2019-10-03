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
			&Server::handleConnection, this,
			connection, std::placeholders::_1));
}

void connectdisks::Server::handleConnection(std::shared_ptr<Connection> connection, const boost::system::error_code & error)
{
#if defined DEBUG || defined _DEBUG
	std::cerr << "Server trying to accept connection\n";
#endif
	if (!error.failed())
	{
		std::lock_guard<std::mutex> lock{lobbiesMutex};
		const auto numLobbies = lobbies.size();
	#if defined DEBUG || defined _DEBUG
		std::cerr << "Server accepted connection, there are " << numLobbies << " lobbies \n";
	#endif
		// assign connection to an existing lobby if one exists
		if (numLobbies != 0)
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
			else // all current lobbies are full
			{
				// only make new lobby if not at cap
				if (numLobbies < maxLobbies)
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
				else
				{
				#if defined DEBUG || defined _DEBUG
					std::cout << "Server at lobby cap, player not added to lobby\n";
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
				std::cout << "Making new lobby and adding player\n";
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
				std::cout << "Server at lobby cap, player not added to lobby\n";
			#endif
			}
		}
	}
	else
	{
	#if defined DEBUG || defined _DEBUG
		std::cerr << "Server::acceptConnection: " << error.message() << "\n";
	#endif
		return;
	}

#if defined DEBUG || defined _DEBUG
	std::cout << "Added player\n";
#endif

	waitForConnections();
}

std::shared_ptr<Server::Connection> connectdisks::Server::Connection::create(boost::asio::io_service & ioService, GameLobby* lobby)
{
	return std::shared_ptr<Connection>{new Connection{ioService, lobby}};
}

void connectdisks::Server::Connection::onGameStart()
{
	// send id to client
	std::shared_ptr<ServerMessage> response{new ServerMessage{}};
	response->response = toScopedEnum<ServerResponse>::cast(
		boost::endian::native_to_big(toUnderlyingType(ServerResponse::gameStart)));

	// send the number of players
	response->data[0] = boost::endian::native_to_big(lobby->getNumPlayers());

	// send the board dimensions
	auto* game = lobby->getGame();
	const auto numCols = boost::endian::native_to_big(game->getNumColumns());
	const auto numRows = boost::endian::native_to_big(game->getNumRows());
	response->data[1] = numCols;
	response->data[2] = numRows;

	boost::asio::async_write(socket,
		boost::asio::buffer(response.get(), sizeof(ServerMessage)),
		std::bind(
			&Connection::handleWrite,
			this,
			response,
			std::placeholders::_1,
			std::placeholders::_2
		));
}

void connectdisks::Server::Connection::onGameEnd()
{
	// tell client game has ended
	std::shared_ptr<ServerMessage> response{new ServerMessage{}};
	response->response = toScopedEnum<ServerResponse>::cast(
		boost::endian::native_to_big(toUnderlyingType(ServerResponse::gameEnd)));
	boost::asio::async_write(socket,
		boost::asio::buffer(response.get(), sizeof(ServerMessage)),
		std::bind(
			&Connection::handleWrite,
			this,
			response,
			std::placeholders::_1,
			std::placeholders::_2
		));
}

void connectdisks::Server::Connection::waitForMessages()
{
#if defined DEBUG || defined _DEBUG
	std::cerr << "Connection " << this << " waiting to read message\n";
#endif
	// read a message from the client, handle in handleRead
	std::shared_ptr<ClientMessage> message{new ClientMessage{}};
	boost::asio::async_read(socket,
		boost::asio::buffer(message.get(), sizeof(ClientMessage)),
		std::bind(
			&Connection::handleRead,
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

		// send id to client
		std::shared_ptr<ServerMessage> response{new ServerMessage{}};
		response->response = toScopedEnum<ServerResponse>::cast(
			boost::endian::native_to_big(toUnderlyingType(ServerResponse::connected)));
		response->data[0] = boost::endian::native_to_big(id);
		boost::asio::async_write(socket,
			boost::asio::buffer(response.get(), sizeof(ServerMessage)),
			std::bind(
				&Connection::handleWrite,
				this,
				response,
				std::placeholders::_1,
				std::placeholders::_2
			));
	}
}

Board::player_size_t connectdisks::Server::Connection::getId() const noexcept
{
	return id;
}

void connectdisks::Server::Connection::setGameLobby(GameLobby * lobby)
{
	this->lobby = lobby;
}

boost::asio::ip::tcp::socket& connectdisks::Server::Connection::getSocket()
{
	return socket;
}

connectdisks::Server::Connection::Connection(boost::asio::io_service & ioService, GameLobby* lobby) :
	lobby{lobby},
	socket{ioService},
	id{0}
{
}

void connectdisks::Server::Connection::handleRead(std::shared_ptr<connectdisks::ClientMessage> message, const boost::system::error_code & error, size_t len)
{
#if defined DEBUG || defined _DEBUG
	std::cerr << "Connection " << this << " trying to read message\n";
#endif
	if (!error.failed())
	{

		if (len == 0)
		{
		#if defined DEBUG || defined _DEBUG
			std::cerr << "Connection " << this << " received 0 length message\n";
		#endif
			return;
		}

		const auto request{
			toScopedEnum<ClientResponse>::cast(
				boost::endian::big_to_native(toUnderlyingType(message->response))
			)
		};
		if (request == ClientResponse::ready)
		{
		#if defined DEBUG || defined _DEBUG
			std::cout << "Connection " << this << ": Client is ready\n";
		#endif
			handleClientReady();
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
			std::cerr << "Connection " << this << "::handleRead: client disconnected \n";
		#endif
			handleDisconnect();
			break;
		default:
		#if defined DEBUG || defined _DEBUG
			std::cerr << "Connection " << this << "::handleRead: " << error.message() << "\n";
		#endif
			break;
		}
	}

}

void connectdisks::Server::Connection::handleWrite(std::shared_ptr<ServerMessage> message, const boost::system::error_code & error, size_t len)
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
		std::cerr << "Connection::handleWrite: " << error.message() << "\n";
	#endif
	}
}

void connectdisks::Server::Connection::handleDisconnect()
{
	lobby->onDisconnect(shared_from_this());
}

void connectdisks::Server::Connection::handleClientReady()
{
	lobby->onReady(shared_from_this());
}

connectdisks::Server::GameLobby::GameLobby(Board::player_size_t maxPlayers) :
	lobbyIsOpen{false},
	canAddPlayers{true},
	isPlayingGame{false},
	maxPlayers{maxPlayers},
	nextId{1},
	numReady{0}
{
	players.reserve(maxPlayers);
}

connectdisks::Server::GameLobby::~GameLobby()
{
}

void connectdisks::Server::GameLobby::start()
{
	startLobby();
}

void connectdisks::Server::GameLobby::onDisconnect(std::shared_ptr<Server::Connection> connection)
{
	if (!lobbyIsOpen)
	{
		return;
	}

	std::lock_guard<std::mutex> lock{playersMutex};
	size_t prevSize = players.size();
	players.erase(std::remove_if(players.begin(), players.end(),
		[&connection](std::shared_ptr<Server::Connection>& con){ return con->getId() == connection->getId(); }));

	if (players.size() != prevSize)
	{
		// connection belongs to lobby
	#if defined DEBUG || defined _DEBUG
		std::cout << "GameLobby " << this << " player disconnected; remaining: " << players.size() << "\n";
	#endif
		--numReady;
		if (isPlayingGame)
		{
			stopGame();
		}

		// todo handle game over plus empty lobby
		/*if (isEmptyInternal())
		{
		}*/
	}

}

void connectdisks::Server::GameLobby::onReady(std::shared_ptr<Server::Connection> connection)
{
	std::unique_lock<std::mutex> lock{playersMutex};
	auto player = std::find_if(players.begin(), players.end(),
		[&connection](std::shared_ptr<Server::Connection>& con){
			return con->getId() == connection->getId();
		});
	if (player != players.end())
	{
		++numReady;
		// connection belongs to lobby
		if (allPlayersAreReady() && isFullInternal())
		{
			game.reset(new ConnectDisks{maxPlayers});
			lock.unlock();
			// start if all players are ready
			startGame();
		}
	}
}

void connectdisks::Server::GameLobby::startGame()
{
	// clumsy - fix with atomic memory order
	std::unique_lock<std::mutex> lock{playersMutex};
	canAddPlayers = false;
	isPlayingGame = true;
	lock.unlock();

	// should now be immutable
	for (auto& player : players)
	{
		player->onGameStart();
	}
}

void connectdisks::Server::GameLobby::stopGame()
{
#if defined DEBUG || defined _DEBUG
	std::cout << "GameLobby " << this << " is stopping game\n";
#endif
	// stop playing if lost a player
	isPlayingGame = false;
	for (auto& player : players)
	{
		player->onGameEnd();
	}
}

void connectdisks::Server::GameLobby::addPlayer(std::shared_ptr<Connection> connection)
{
	std::lock_guard<std::mutex> lock{playersMutex};
	players.push_back(connection);
	connection->setId(nextId++);
	connection->setGameLobby(this);
	connection->waitForMessages();
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

Board::player_size_t connectdisks::Server::GameLobby::getNumPlayers() const noexcept
{
	std::lock_guard<std::mutex> lock{playersMutex};
	return static_cast<Board::player_size_t>(players.size());
}

ConnectDisks * connectdisks::Server::GameLobby::getGame() const noexcept
{
	return game.get();
}

void connectdisks::Server::GameLobby::startLobby()
{
	lobbyIsOpen = true;
#if defined DEBUG || defined _DEBUG
	std::cout << "GameLobby " << this << " has started\n";
#endif
}

bool connectdisks::Server::GameLobby::isEmptyInternal() const noexcept
{
	return players.empty();
}

bool connectdisks::Server::GameLobby::isFullInternal() const noexcept
{
	return players.size() == maxPlayers;
}

bool connectdisks::Server::GameLobby::allPlayersAreReady() const noexcept
{
	return numReady == players.size();
}
