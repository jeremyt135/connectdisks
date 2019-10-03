#pragma once

#include "connectdisks/board.hpp"
#include "connectdisks/connectdisks.hpp"

#include "connectdisks/messaging.hpp"

#include <boost/asio.hpp>

#include <future>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace connectdisks
{
	// Accepts connections from clients wanting to play ConnectDisks 
	// and manages game lobbies.
	class Server
	{
	public:
		Server(
			boost::asio::io_service& ioService,
			std::string address, 
			uint16_t port);
		Server(const Server&) = delete;
		~Server();

		Server& operator=(const Server&) = delete;

	private:
		static constexpr Board::player_size_t maxLobbies{4};

		class GameLobby; // Runs a ConnectDisks game
		class Connection; // Maintains connection from client

		void waitForConnections();
		void handleConnection(std::shared_ptr<Connection> connection, const boost::system::error_code& error);

		mutable std::mutex lobbiesMutex;

		boost::asio::io_service& ioService;
		boost::asio::ip::tcp::acceptor acceptor;

		std::vector<std::unique_ptr<GameLobby>> lobbies;
	};

	// Runs a ConnectDisks game
	class Server::GameLobby
	{
	public:
		GameLobby(Board::player_size_t maxPlayers = ConnectDisks::minNumPlayers);
		~GameLobby();

		// Starts the lobby and waits for enough players to start a game
		void start();

		// Adds a player (client connection) to the game lobby
		void addPlayer(std::shared_ptr<Connection> connection);

		// Returns true if no players are connected
		bool isEmpty() const noexcept;

		// Returns true if the max number of players are connected
		bool isFull() const noexcept;

		Board::player_size_t getNumPlayers() const noexcept;

		ConnectDisks* getGame() const noexcept;

		// TODO - Finalize signal/slot for Connection ending and being removed from lobby
		void onDisconnect(std::shared_ptr<Server::Connection> connection);

		// TODO - Finalize signal/slot interface
		void onReady(std::shared_ptr<Server::Connection> connection);
	private:
		void startGame(); 
		void stopGame();
		void startLobby(); 

		bool isEmptyInternal() const noexcept;
		bool isFullInternal() const noexcept;
		bool allPlayersAreReady() const noexcept;

		std::atomic<bool> lobbyIsOpen;
		std::atomic<bool> isPlayingGame;
		std::atomic<bool> canAddPlayers;

		mutable std::mutex playersMutex;

		std::unique_ptr<ConnectDisks> game;
		Board::player_size_t maxPlayers;
		Board::player_size_t nextId;
		Board::player_size_t numReady;
		std::vector<std::shared_ptr<Connection>> players;
	};

	// Maintains connection from clients
	class Server::Connection : public std::enable_shared_from_this<Server::Connection>
	{
	public:
		// TODO - Finalize passing method/subscription for disconnecting from lobby
		static std::shared_ptr<Server::Connection> create(boost::asio::io_service& ioService, GameLobby* lobby = nullptr);

		// TODO - Finalize passing method/subscription for disconnecting from lobby
		void onGameStart();
		void onGameEnd();

		// Starts an async read from the socket
		void waitForMessages();

		// Sets the id that the player should have
		void setId(Board::player_size_t id);

		Board::player_size_t getId() const noexcept;

		// Sets the GameLobby that this is connected to
		void setGameLobby(GameLobby* lobby);

		boost::asio::ip::tcp::socket& getSocket();
	private:
		Connection(boost::asio::io_service& ioService, GameLobby* lobby = nullptr);

		// Handles messages from the client on other end of connection
		void handleRead(std::shared_ptr<ClientMessage> message, const boost::system::error_code& error, size_t len);

		// Sends message to client
		void handleWrite(std::shared_ptr<ServerMessage> message, const  boost::system::error_code& error, size_t len);

		void handleDisconnect();
		void handleClientReady();
		
		GameLobby* lobby;

		boost::asio::ip::tcp::socket socket;
		Board::player_size_t id;
	};


}
