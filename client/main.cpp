#include "four-across/networking/client/client.hpp"
#include "four-across/networking/client/consoleclient.hpp"

#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>

#include <iostream>
#include <string>
#include <thread>

using namespace game::networking::client;

struct Options
{
	std::string host;
	uint16_t port;
};

void runClient(const Options& options){
	try
	{
		std::unique_ptr<Client> gameClient{new ConsoleClient{}};
		gameClient->connect(options.host, options.port);
	}
	catch (std::exception &e)
	{
		std::cerr << "An error occurred while running the client: " << e.what() << "\n";
	}
}
const char* const usage = "Usage: client HOST PORT";
void printUsage() {
	std::cout << usage << std::endl;
}

Options getOptions(int argc, char *argv[])
{
	if (argc != 3) {
		printUsage();
		exit(EXIT_FAILURE);
	}

	Options options{};
	try{
		// ensure host is valid address
		boost::system::error_code error;
		boost::asio::ip::address_v4::from_string(argv[1], error);
		if (error){
			throw std::exception();
		}
		options.host = argv[1];
		options.port = boost::lexical_cast<uint16_t>(argv[2]);
	}catch(std::exception&){
		printUsage();
		exit(EXIT_FAILURE);
	}

	return options;
}


int main(int argc, char *argv[])
{
	runClient(getOptions(argc, argv));
}
