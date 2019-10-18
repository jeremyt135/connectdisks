#pragma once

#include <array>
#include <cstdint>

namespace game
{
	namespace networking
	{
		enum class MessageType : uint8_t
		{
			/*
				Some error state occurred
			*/
			error,

			/*
				Client successfully connected:
				data[0]: Client's player id for the game
			*/
			connected,
			/*
				Server checking if Client is alive
			*/
			ping,
			/*
				Client confirming they are alive
			*/
			pong,
			/*
				Client connected to server but is in
				queue for a game lobby.
				- first 8 bytes of data contains position
			*/
			inQueue,

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
				Client is ready to play
				data[0]: client id
			*/
			ready,

			/*
				Server wants a client to move or client has moved
				data[0]: client id
				data[1]: column (-1 if requesting turn)
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
				data[0]: client id
				data[1]: column
			*/
			update,
		};
		/*
			Contains data to pass from server to client
		*/
		struct Message
		{
			MessageType type;
			std::array<uint8_t, 16> data;
		};
	}
}
