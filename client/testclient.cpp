#include "four-across/networking/client/client.hpp"

#include <boost/asio.hpp>

#include <iostream>
#include <thread>

using game::networking::client::Client;
using game::FourAcross;

std::unique_ptr<Client> gameClient;
std::unique_ptr<boost::asio::io_service> ioService;

// Sets up and runs gameClient
void runClient();

/*
	Callbacks to use with gameClient
*/

void onConnect(uint8_t id);
void onQueuePositionUpdated(uint64_t position);
void onDisconnect();

void onGameStart(uint8_t numPlayers, uint8_t first, uint8_t cols, uint8_t rows);
void onGameEnd(uint8_t winner);

void onTakeTurn();
void onTurnResult(FourAcross::TurnResult result, uint8_t column);

void onUpdate(uint8_t player, uint8_t col);

/*
	User input prompts
*/

std::future<void> readAsync;
std::atomic<bool> cancelRead{false};

std::function<bool(std::string)> inputTask;

// Reads input continuously from cin
void readInput();

// Asks if user is ready to play
void getUserReady();
bool readUserReady(std::string input);

// Asks if user wants to rematch (stay in same lobby)
void getUserRematch();
bool readUserRematch(std::string input);

// Ask for player's turn (column)
void getUserColumn();
bool readUserColumn(std::string input);

int main(int argc, char* argv[])
{
	try
	{
		ioService.reset(new boost::asio::io_service{});
		readAsync = std::async(std::launch::async, readInput);
		runClient();
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << "\n";
	}
	cancelRead = true;
	readAsync.get();
	std::cout << "Press enter to exit..." << std::endl;
	std::cin.clear();
	std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

void runClient()
{
	try
	{
		gameClient.reset(new Client{*ioService});

		gameClient->addConnectHandler(onConnect);
		gameClient->addQueueUpdateHandler(onQueuePositionUpdated);
		gameClient->addDisconnectHandler(onDisconnect);

		gameClient->addGameStartHandler(onGameStart);
		gameClient->addGameEndHandler(onGameEnd);

		gameClient->addTurnRequestHandler(onTakeTurn);
		gameClient->addTurnResultHandler(onTurnResult);
		gameClient->addGameUpdateHandler(onUpdate);

		gameClient->connectToServer("127.0.0.1", 8888);
		ioService->run();
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << "\n";
	}
}

void readInput()
{
	while (!cancelRead)
	{
		std::string line;
		std::getline(std::cin, line);
		if (inputTask)
		{
			bool done = inputTask(line);
			// input consumed, task done
			if (done)
			{
				inputTask = nullptr;
			}
		}
	}
}

void getUserReady()
{
	std::cout << "Input \"ready\" if you want to play, or \"quit\" to disconnect\n";
	inputTask = readUserReady;
}

bool readUserReady(std::string input)
{
	bool ready;
	if (input == "ready")
	{
		std::cout << "Sending ready to server\n";
		ready = true;
	}
	else if (input == "quit")
	{
		ready = false;
	}
	else
	{
		std::cout << "Input \"ready\" if you want to play, or \"quit\" to disconnect\n";
		return false;
	}

	if (ready)
	{
		gameClient->toggleReady();
	}
	else
	{
		ioService->stop();
		ioService.reset();
	}

	return true;
}

void getUserRematch()
{
	std::cout << "Input \"rematch\" if you want to play, or \"quit\" to disconnect\n";
	inputTask = readUserRematch;
}

bool readUserRematch(std::string input)
{
	bool rematch = false;
	if (input == "rematch")
	{
		std::cout << "Sending rematch to server\n";
		rematch = true;
	}
	else if (input == "quit")
	{
		rematch = false;
	}
	else
	{
		std::cout << "Input \"rematch\" if you want to play, or \"quit\" to disconnect\n";
		return false;
	}

	if (rematch)
	{
		gameClient->toggleReady();
	}
	else
	{
		ioService->stop();
		ioService.reset();
	}
	return true;
}

void getUserColumn()
{
	inputTask = readUserColumn;
}

bool readUserColumn(std::string input)
{
	const auto maxColumns = static_cast<int>(gameClient->getGame()->getNumColumns());

	int column{-1};
	try
	{
		size_t pos = 0;
		column = std::stoi(input, &pos);
		if (pos == input.size() && column > 0 && column <= maxColumns)
		{
			// read successful, take turn
			gameClient->takeTurn(column - 1);
			return true;
		}
	}
	catch (...) // invalid_argument or out_of_range
	{
	}
	std::cout << "Invalid input, try again: ";
	
	return false;
}

void onConnect(uint8_t id)
{
	std::cout << "You have connected to the game server. Your id is " << static_cast<int>(id) << "\n";
	// ask if user is ready and quit if not
	getUserReady();
}

void onQueuePositionUpdated(uint64_t position)
{
	std::cout << "Server is full, you are in position " << position << "\n";
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
	getUserRematch();
}

void onTakeTurn()
{
	std::cout << *gameClient->getGame() << "\n";
	std::cout << "It's your turn!\n"
		<< "Enter the column to place your piece (min=1,max=" <<
		static_cast<int>(gameClient->getGame()->getNumColumns()) <<
		"): ";

	getUserColumn();
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
