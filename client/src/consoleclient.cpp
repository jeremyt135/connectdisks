#include "four-across/networking/client/consoleclient.hpp"

#include <iostream>

using namespace game::networking::client;

ConsoleClient::ConsoleClient()
{
}

void ConsoleClient::onConnect(uint8_t playerId)
{
	Client::onConnect(playerId);
	std::cout << "You have connected to the game server. Your id is " << static_cast<int>(playerId) << "\n";
	std::cout << "Input \"ready\" if you want to play, or \"quit\" to disconnect:" << std::endl;

	// Allow user to confirm when they are ready or want to quit
	inputWorker.start(
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

void ConsoleClient::onDisconnect()
{
	Client::onDisconnect();
	// Stop reading input
	std::cout << "You have disconnected from the game server. Enter anything to exit...\n";
	inputWorker.stop();
}

void ConsoleClient::onQueueUpdate(uint64_t queuePosition)
{
	Client::onQueueUpdate(queuePosition);
	std::cout << "Server is full, you are in position " << queuePosition << "\n";
}


void ConsoleClient::onGameStart(uint8_t numPlayers, uint8_t firstPlayer, uint8_t cols, uint8_t rows)
{
	// Display info about the game that just started
	std::cout << "Your game has started with " << static_cast<int>(numPlayers) << " players.\n"
		<< "Player " << static_cast<int>(firstPlayer) << " is first.\n"
		<< "The board size is " << static_cast<int>(cols) << "x" << static_cast<int>(rows) << "\n";
}

void ConsoleClient::onGameEnd(uint8_t winner)
{
	const uint8_t noWinner = 0;
	if (static_cast<int>(winner) == noWinner){
		std::cout << "The game has ended because your opponent disconnected." << std::endl;
	}
	else {
		std::cout << "The game is over, player" << static_cast<int>(winner) << " has won" << std::endl;	
	}
	std::cout << "Input \"rematch\" if you want to rematch, or \"quit\" to quit:" << std::endl;

	// Allow user to confirm if they want a rematch or to quit
	inputWorker.restart(
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

void ConsoleClient::onGameUpdate(uint8_t player, uint8_t col)
{
	// Display the new state of the game
	std::cout << *this->getGame() << "\n";
	// Show the player that did the move
	std::cout << "Player " << static_cast<int>(player) << " dropped a piece in column " <<
		static_cast<int>(col) + 1 << "\n";
}

void ConsoleClient::onTurnResult(FourAcross::TurnResult result, uint8_t column)
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

void ConsoleClient::handleTurnRequest()
{
	// Display the current game state
	std::cout << *this->getGame() << "\n";
	std::cout << "It's your turn!\n"
		<< "Enter the column to place your piece (min=1,max=" <<
		static_cast<int>(this->getGame()->getNumColumns()) <<
		"): ";

	const auto maxColumns = static_cast<int>(this->getGame()->getNumColumns());
	// Prompt user for their turn
	inputWorker.restart(
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


ConsoleClient::InputWorker::~InputWorker()
{
	stop();
}

void ConsoleClient::InputWorker::start(ConsoleClient::InputWorker::InputCallback callback)
{
	if (running)
	{
		return;
	}
	running = true;
	thread = std::thread(&InputWorker::readUntilDone, this, callback);
}


void ConsoleClient::InputWorker::restart(ConsoleClient::InputWorker::InputCallback callback)
{
	if (running)
	{
		stop();
	}
	start(callback);
}

void ConsoleClient::InputWorker::stop()
{
	if (!running)
	{
		return;
	}
	running = false;
	thread.join();
}

bool ConsoleClient::InputWorker::isRunning(){
	return running;
}

void ConsoleClient::InputWorker::readUntilDone(InputCallback callback)
{
	while (running)
	{
		// Read continuously from stdin until callback indicates good input received.
		std::string input;
		std::cin >> input;
		if (running && callback(input))
		{
			break;
		}
	}
}

