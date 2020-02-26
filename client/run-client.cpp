#include "four-across/networking/client/client.hpp"
#include "four-across/networking/client/consoleclient.hpp"

#include <boost/asio.hpp>

#include <iostream>
#include <thread>

using namespace game::networking::client;

int main(int argc, char* argv[])
{
	try
	{
		std::unique_ptr<Client> gameClient{new ConsoleClient{}};
		gameClient->connect("127.0.0.1", 8888);
	}
	catch (std::exception& e)
	{
		std::cerr << "An error occurred while running the client: " << e.what() << "\n";
	}
}
