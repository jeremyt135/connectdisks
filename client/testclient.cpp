#include "four-across/networking/client/client.hpp"

#include <boost/asio.hpp>

#include <iostream>
#include <thread>

using game::networking::client::Client;
using game::FourAcross;

std::unique_ptr<Client> gameClient;

// Sets up and runs gameClient
void runClient();

/*
	Callbacks to use with gameClient
*/

void onConnect(uint8_t id);
void onDisconnect();

void onGameStart(uint8_t numPlayers, uint8_t first, uint8_t cols, uint8_t rows);
void onGameEnd(uint8_t winner);

void onTakeTurn();
void onTurnResult(FourAcross::TurnResult result, uint8_t column);

void onUpdate(uint8_t player, uint8_t col);

/*
	User input prompts
*/

// Asks if user is ready to play
bool getUserReady();

// Asks if user wants to rematch (stay in same lobby)
bool getUserRematch();

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
	std::cout << "Press enter to exit..." << std::endl;
	std::cin.clear();
	std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
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

		gameClient->addTurnRequestHandler(onTakeTurn);
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

bool getUserReady()
{
	std::string input;
	for (;;)
	{
		std::cout << "Input \"ready\" if you want to play, or \"quit\" to disconnect\n";
		std::getline(std::cin, input);
		if (input == "ready")
		{
			std::cout << "Sending ready to server\n";
			return true;
		}
		else if (input == "quit")
		{
			std::cout << "Sending quit to server\n";
			return false;
		}
	}
}

bool getUserRematch()
{
	std::string input;
	for (;;)
	{
		std::cout << "Input \"rematch\" if you want to play, or \"quit\" to disconnect\n";
		std::getline(std::cin, input);
		if (input == "rematch")
		{
			std::cout << "Sending rematch to server\n";
			return true;
		}
		else if (input == "quit")
		{
			std::cout << "Sending quit to server\n";
			return false;
		}
	}
}

void onConnect(uint8_t id)
{
	std::cout << "You have connected to the game server. Your id is " << static_cast<int>(id) << "\n";
	// ask if user is ready and quit if not
	auto ready = getUserReady();
	if (ready)
	{
		gameClient->toggleReady();
	}
	else
	{
		gameClient->disconnect();
	}
}

void onDisconnect()
{
	std::cout << "You have disconnected from the game server.\n";
	
	// can either connect to server again, or exit process
	exit(EXIT_SUCCESS); // for now just exit
}

void onGameStart(uint8_t numPlayers, uint8_t first,
	uint8_t cols, uint8_t rows)
{
	std::cout << "Your game has started with " << static_cast<int>(numPlayers) << " players.\n"
		<< "Player " << static_cast<int>(first) << " is first.\n"
		<< "The board size is " << static_cast<int>(cols) << "x" << static_cast<int>(rows) << "\n";
}

void onGameEnd(uint8_t winner)
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

	// ask if user wants to play again
	bool rematch = getUserRematch();
	if (rematch)
	{
		// indicate that user is ready to play, stay in lobby
		gameClient->toggleReady();
	}
	else
	{
		// disconnect from lobby
		gameClient->disconnect();
	}
}

void onTakeTurn()
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
	
	gameClient->takeTurn(column - 1);
}

void onTurnResult(FourAcross::TurnResult result, uint8_t column)
{
	using TurnResult = FourAcross::TurnResult;
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

void onUpdate(uint8_t player, uint8_t col)
{
	std::cout << *gameClient->getGame() << "\n";
	std::cout << "Player " << static_cast<int>(player) << " dropped a piece in column " <<
		static_cast<int>(col) + 1 << "\n";
}
