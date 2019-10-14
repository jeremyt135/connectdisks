#include "connectdisks/client.hpp"

#include <boost/asio.hpp>

#include <iostream>
#include <thread>

using namespace connectdisks;
using connectdisks::client::Client;

std::unique_ptr<Client> gameClient;

/*
	Callbacks to use with gameClient
*/
void runClient();

void onConnect(Board::player_size_t id);
void onDisconnect();

void onGameStart(Board::player_size_t numPlayers, Board::player_size_t first, 
				 Board::board_size_t cols, Board::board_size_t rows);
void onGameEnd(Board::player_size_t winner);

Board::board_size_t onTakeTurn();
void onTurnResult(ConnectDisks::TurnResult result, Board::board_size_t column);

void onUpdate(Board::player_size_t player, Board::board_size_t col);

int main(int argc, char* argv[])
{
	try
	{
		runClient();
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << "\n";
	}
	int i;
	std::cin >> i;
}

void runClient()
{
	try
	{
		boost::asio::io_service service;
		gameClient.reset(new Client{service});
		gameClient->addConnectHandler(onConnect);
		gameClient->addDisconnectHandler(onDisconnect);
		gameClient->addGameStartHandler(onGameStart);
		gameClient->addGameEndHandler(onGameEnd);
		gameClient->addTurnHandler(onTakeTurn);
		gameClient->addTurnResultHandler(onTurnResult);
		gameClient->addGameUpdateHandler(onUpdate);
		gameClient->connectToServer("127.0.0.1", 8888);
		service.run();
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << "\n";
	}
}

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
		std::cout << "You can exit this window to leave, or wait for a new player\n";
	}
	else
	{
		std::cout << "Player " << static_cast<int>(winner) << " won\n";
		std::cout << "You can exit this window to leave\n";
	}

}

Board::board_size_t onTakeTurn()
{
	std::cout << *gameClient->getGame() << "\n";
	std::cout << "It's your turn!\n"
		<< "Enter the column to place your piece (min=1,max=" <<
		static_cast<int>(gameClient->getGame()->getNumColumns()) <<
		"): ";

	const auto maxColumns = static_cast<int>(gameClient->getGame()->getNumColumns());

	
	// read line from cin until a valid column is read
	int column{-1};
	std::string input;
	for (;;)
	{
		std::getline(std::cin, input);
		try
		{
			size_t pos = 0;
			column = std::stoi(input, &pos);
			if (pos == input.size() && column > 0 && column <= maxColumns)
			{
				break;
			}
		}
		catch (...) // invalid_argument or out_of_range
		{
		}
		std::cout << "Invalid input, try again: ";
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
		std::cout << "Your move was accepted\n" << *gameClient->getGame() << "\n";
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
	std::cout << *gameClient->getGame() << "\n";
	std::cout << "Player " << static_cast<int>(player) << " dropped a piece in column " <<
		static_cast<int>(col) + 1 << "\n";
}
