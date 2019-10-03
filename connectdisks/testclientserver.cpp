#include "connectdisks/connectdisks.hpp"
#include "connectdisks/client.hpp"
#include "connectdisks/server.hpp"

#include <boost/asio.hpp>

#include <iostream>
#include <thread>

using namespace connectdisks;

std::promise<void> serverIsReady;
std::shared_future<void> serverReadyFuture;
void runServer()
{
	try
	{
		boost::asio::io_service service;
		Server server{service, "127.0.0.1", 8888};
		serverIsReady.set_value();
		service.run();
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << "\n";
	}
}

void runClient(int id)
{
	try
	{
		serverReadyFuture.wait();
		std::cout << "starting client " << id << "\n";
		boost::asio::io_service service;
		Client client{service, "127.0.0.1", 8888};
		std::cout << "running client " << id << "\n";
		service.run();
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << "\n";
	}
}

int main(int argc, char* argv[])
{
	try
	{
		serverReadyFuture = serverIsReady.get_future();

		auto server = std::async(std::launch::async, runServer);
		// launch clients
		auto client1 = std::async(std::launch::async, runClient, 1);
		auto client2 = std::async(std::launch::async, runClient, 2);

	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << "\n";
	}
}
