#pragma once

#include <array>
#include <cstdint>

namespace connectdisks
{
	enum class ServerResponse : uint8_t
	{
		error, connected, gameStart, gameEnd
	};

	struct ServerMessage
	{
		ServerResponse response;
		std::array<uint8_t, 16> data;
	};


	enum class ClientResponse : uint8_t
	{
		error, ready
	};

	struct ClientMessage
	{
		ClientResponse response;
		std::array<uint8_t, 16> data;
	};
}
