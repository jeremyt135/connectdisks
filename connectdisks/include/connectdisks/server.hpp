#pragma once

#include "connectdisks/board.hpp"
#include "connectdisks/connectdisks.hpp"

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
		enum class Response : uint8_t;

		Server(
			boost::asio::io_service& ioService,
			std::string address, 
			uint16_t port);
		Server(const Server&) = delete;
		~Server();

		Server& operator=(const Server&) = delete;

	private:
		class GameLobby; // Runs a ConnectDisks game
		class Connection; // Maintains connection from client

		void waitForConnections();
		void acceptConnection(std::shared_ptr<Connection> connection, const boost::system::error_code& error);

		boost::asio::io_service& ioService;
		boost::asio::ip::tcp::acceptor acceptor;

		std::vector<std::unique_ptr<GameLobby>> lobbies;
	};

	enum class Server::Response : uint8_t
	{

	};

	struct ServerMessage
	{
		Server::Response response;
		std::array<uint8_t, 3> data;
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
	private:
		void startGame(); // signal the start of a game
		void startLobby(); // start the lobby and wait for players

		bool isEmptyInternal() const noexcept;
		bool isFullInternal() const noexcept;

		std::future<void> lobby;
		std::atomic<bool> shouldClose;
		std::atomic<bool> canAddPlayers;

		mutable std::mutex playersMutex;

		std::promise<void> startGameSignal;

		std::unique_ptr<ConnectDisks> game;
		Board::player_size_t maxPlayers;
		std::vector<std::shared_ptr<Connection>> players;
	};

	struct ClientMessage;

	// Maintains connection from clients
	class Server::Connection : public std::enable_shared_from_this<Server::Connection>
	{
	public:
		static std::shared_ptr<Server::Connection> create(boost::asio::io_service& ioService);

		// Starts an async read from the socket
		void waitForMessages();

		// Sets the id that the player should have
		void setId(Board::player_size_t id);

		boost::asio::ip::tcp::socket& getSocket();
	private:
		Connection(boost::asio::io_service& ioService);

		// Handles messages from the client on other end of connection
		void readMessage(std::shared_ptr<ClientMessage> message, const boost::system::error_code& error, size_t len);

		boost::asio::ip::tcp::socket socket;
		Board::player_size_t id;
	};

	
}
