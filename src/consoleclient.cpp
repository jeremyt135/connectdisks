#include "four-across/networking/client/consoleclient.hpp"

#include <iostream>

game::networking::client::ConsoleClient::ConsoleClient()
{
	inputWorker.start();
}

void game::networking::client::ConsoleClient::onConnect(uint8_t playerId)
{
	Client::onConnect(playerId);
	std::cout << "You have connected to the game server. Your id is " << static_cast<int>(playerId) << "\n";
	std::cout << "Input \"ready\" if you want to play, or \"quit\" to disconnect:" << std::endl;

	// Allow user to confirm when they are ready or want to quit
	inputWorker.setTask(
		[this](std::string input){
			if (input == "ready")
			{
				std::cout << "Sending ready to server\n";
				this->toggleReady();
				return true;
			}
			else if (input == "quit")
			{
				std::cout << "Exiting" << std::endl;
				this->stop();
			}
			else
			{
				std::cout << "Unrecognized input" << std::endl;
			}
			return false;
		}
	);
}

void game::networking::client::ConsoleClient::onDisconnect()
{
	Client::onDisconnect();
	// Stop reading input
	std::cout << "You have disconnected from the game server. Enter anything to exit...\n";
	inputWorker.clearTask();
	inputWorker.stop();
}

void game::networking::client::ConsoleClient::onQueueUpdate(uint64_t queuePosition)
{
	Client::onQueueUpdate(queuePosition);
	std::cout << "Server is full, you are in position " << queuePosition << "\n";
}


void game::networking::client::ConsoleClient::onGameStart(uint8_t numPlayers, uint8_t firstPlayer, uint8_t cols, uint8_t rows)
{
	// Display info about the game that just started
	std::cout << "Your game has started with " << static_cast<int>(numPlayers) << " players.\n"
		<< "Player " << static_cast<int>(firstPlayer) << " is first.\n"
		<< "The board size is " << static_cast<int>(cols) << "x" << static_cast<int>(rows) << "\n";
}

void game::networking::client::ConsoleClient::onGameEnd(uint8_t winner)
{
	std::cout << "The game is over, player" << static_cast<int>(winner) << " has won" << std::endl;
	std::cout << "Input \"rematch\" if you want to rematch, or \"quit\" to quit:" << std::endl;

	// Allow user to confirm if they want a rematch or to quit
	inputWorker.setTask(
		[this](std::string input){
			if (input == "rematch")
			{
				std::cout << "Sending rematch request to server\n";
				this->toggleReady();
				return true;
			}
			else if (input == "quit")
			{
				std::cout << "Exiting..." << std::endl;
				this->stop();
			}
			else
			{
				std::cout << "Unrecognized input" << std::endl;
			}
			return false;
		}
	);
}

void game::networking::client::ConsoleClient::onGameUpdate(uint8_t player, uint8_t col)
{
	// Display the new state of the game
	std::cout << *this->getGame() << "\n";
	// Show the player that did the move
	std::cout << "Player " << static_cast<int>(player) << " dropped a piece in column " <<
		static_cast<int>(col) + 1 << "\n";
}

void game::networking::client::ConsoleClient::onTurnResult(FourAcross::TurnResult result, uint8_t column)
{
	using TurnResult = FourAcross::TurnResult;
	switch (result)
	{
	// successful turn
	case TurnResult::success:
		std::cout << "Your move was accepted\n" << *this->getGame() << "\n";
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

void game::networking::client::ConsoleClient::handleTurnRequest()
{
	// Display the current game state
	std::cout << *this->getGame() << "\n";
	std::cout << "It's your turn!\n"
		<< "Enter the column to place your piece (min=1,max=" <<
		static_cast<int>(this->getGame()->getNumColumns()) <<
		"): ";

	const auto maxColumns = static_cast<int>(this->getGame()->getNumColumns());
	// Prompt user for their turn
	inputWorker.setTask(
		[this, maxColumns](std::string input){
			int column{-1};
			try
			{
				size_t pos = 0;
				column = std::stoi(input, &pos);
				if (pos == input.size() && column > 0 && column <= maxColumns)
				{
					// read successful, take turn
					this->sendTurn(column - 1);
					return true;
				}
			}
			catch (...) // invalid_argument or out_of_range
			{
			}
			std::cout << "Invalid input, try again: ";

			return false;
		}
	);
}


game::networking::client::ConsoleClient::InputWorker::~InputWorker()
{
	stop();
}

void game::networking::client::ConsoleClient::InputWorker::clearTask()
{
	if (!running)
	{
		return;
	}
	
	if (hasTask)
	{
		std::lock_guard<std::mutex> lock(taskMutex);
		hasTask = false;
		this->task = nullptr;
	}
}

void game::networking::client::ConsoleClient::InputWorker::setTask(InputTask task)
{
	if (!running)
	{
		return;
	}
	{
		std::lock_guard<std::mutex> lock(taskMutex);
		this->task = task;
		hasTask = true;
	}
}

void game::networking::client::ConsoleClient::InputWorker::start()
{
	if (running)
	{
		return;
	}
	thread = std::thread(&InputWorker::run, this);
}

void game::networking::client::ConsoleClient::InputWorker::stop()
{
	if (!running)
	{
		return;
	}
	clearTask();
	running = false;
	thread.join();
}

void game::networking::client::ConsoleClient::InputWorker::run()
{
	running = true;
	while (running)
	{
		if (hasTask)
		{
			// Read from stdin when there is a task to do - if the task is cleared 
			// the user still has to input something but it will be safely ignored.
			std::string input;
			std::cin >> input;
			std::lock_guard<std::mutex> lock(taskMutex);
			if (running && hasTask && task(input))
			{
				hasTask = false;
				task = nullptr;
			}
		}
	}
}
