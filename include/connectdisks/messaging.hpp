#pragma once

#include <array>
#include <cstdint>

namespace connectdisks
{
	namespace server
	{
		enum class Response : uint8_t
		{
			error, connected, gameStart, gameEnd, takeTurn, turnResult, update
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
		enum class Response : uint8_t
		{
			error, ready, turn
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
