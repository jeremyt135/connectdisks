#pragma once

#include <array>
#include <cstdint>

namespace connectdisks
{
	namespace server
	{
		// Type of message sent to Client from Server
		enum class Response : uint8_t
		{
			/*
				Some error state occurred in server
			*/
			error,		
			/*
				Client successfully connected:
				data[0]: Client's player id for the game
			*/
			connected,
			/*
				Game lobby has started:
				data[0]: number of players
				data[1]: number of board columns
				data[2]: number of board rows
				data[3]: first player to move
			*/
			gameStart, 
			/*
				Game lobby has ended:
				data[0]: id of winning player
			*/
			gameEnd, 
			/*
				Client should take their turn
			*/
			takeTurn, 
			/*
				Game lobby has result of client's turn taken:
				data[0]: turn result as type ConnectDisks::TurnResult
				data[1]: column
			*/
			turnResult, 
			/*
				Game lobby has result of opponent client's turn taken:
				data[0]: playerId
				data[1]: column
			*/
			update, 
			/*
				Game lobby wants to know if client is staying in lobby for rematch:
				data[0]: 1 (true) if contains data from Client
				data[1]: 1 (true) if Client wants to rematch
			*/
			rematch
		};
		/*
			Contains data to pass from server to client
		*/
		struct Message
		{
			Response response;
			std::array<uint8_t, 16> data;
		};
	}

	namespace client
	{
		// Type of message sent to Server from Client
		enum class Response : uint8_t
		{
			/*
				Some error state occurred in Client
			*/
			error, 
			/*
				Client is ready to play
			*/
			ready, 
			/*
				Client has their turn:
				data[0]: column they want to move in
			*/
			turn, 
			/*
				Client wants to rematch (similar to server::Response):
				data[0]: 1 (true), error if 0
				data[1]: 1 (true) if Client wants to rematch
			*/
			rematch
		};

		/*
			Contains data to pass from client to server
		*/
		struct Message
		{
			Response response;
			std::array<uint8_t, 16> data;
		};
	}
}
