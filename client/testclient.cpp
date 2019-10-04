#include "connectdisks/client.hpp"

#include <boost/asio.hpp>

#include <iostream>
#include <thread>

using namespace connectdisks;
using connectdisks::client::Client;

std::unique_ptr<Client> gameClient;

void onConnect(Board::player_size_t id)
{
	std::cout << "You have connected to the game server. Your id is " << static_cast<int>(id) << "\n";
}

void onDisconnect()
{
	std::cout << "You have disconnected from the game server.\n";
}

void onGameStart(Board::player_size_t numPlayers, Board::player_size_t first,
	Board::board_size_t cols, Board::board_size_t rows)
{
	std::cout << "Your game has started with " << static_cast<int>(numPlayers) << " players.\n"
		<< "Player " << static_cast<int>(first) << " is first.\n"
		<< "The board size is " << static_cast<int>(cols) << "x" << static_cast<int>(rows) << "\n";
}

void onGameEnd(Board::player_size_t winner)
{
	std::cout << "Your game has ended.\n";
	if (winner == 0)
	{
		std::cout << "(A player disconnected mid-game)\n";
	}
	else
	{
		std::cout << "Player " << static_cast<int>(winner) << " won\n";
	}
}

Board::board_size_t onTakeTurn()
{
	std::cout << "It's your turn!\n"
		<< "Enter the column to place your piece (min=1,max=" <<  
		static_cast<int>(gameClient->getGame()->getNumColumns()) << 
		"): ";
	int column;
	std::cin >> column;
	while (!std::cin.good() && column <= 0 && column > static_cast<int>(gameClient->getGame()->getNumColumns()))
	{
		std::cout << "Invalid input, try again: ";
		std::cin.clear();
		std::cin >> column;
	}
	return static_cast<Board::board_size_t>(column) - 1;
}

void onTurnResult(ConnectDisks::TurnResult result, Board::board_size_t column)
{
	using TurnResult = ConnectDisks::TurnResult;
	switch (result)
	{
	// successful turn
	case TurnResult::success:
		std::cout << "Your move was accepted\n";
		break;

	// error cases
	case TurnResult::badColumn:
		std::cout << "Error: your move was not a valid column\n";
		break;
	case TurnResult::columnFull:
		std::cout << "Error: your move was in a full column\n";
		break;
	case TurnResult::gameFinished:
		std::cout << "Error: your move wasn't counted as the game is over\n";
		break;
	case TurnResult::wrongPlayer:
		std::cout << "Error: it's not your turn\n";
		break;
	default:
		std::cout << "Your turn was not accepted\n";
		break;
	}
}

void onUpdate(Board::player_size_t player, Board::board_size_t col)
{
	std::cout << "Player " << static_cast<int>(player) << " dropped a piece in column " <<
		static_cast<int>(col) << "\n";
}

void runClient()
{
	try
	{
		boost::asio::io_service service;
		gameClient.reset(new Client{service, "127.0.0.1", 8888});
		gameClient->connectHandler = onConnect;
		gameClient->gameStartHandler = onGameStart;
		gameClient->gameEndHandler = onGameEnd;
		gameClient->turnHandler = onTakeTurn;
		gameClient->turnResultHandler = onTurnResult;
		gameClient->updateHandler = onUpdate;
		service.run();
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << "\n";
	}
}

int main(int argc, char* argv[])
{
	try
	{
		auto client = std::async(std::launch::async, runClient);
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << "\n";
	}
	std::cin.clear();
	std::cin.ignore();
}
