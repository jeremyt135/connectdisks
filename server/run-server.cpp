#include "four-across/networking/server/server.hpp"

#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>

#include <iostream>

using game::networking::server::Server;

struct Options
{
	uint16_t port;
};

void runServer(const Options &options)
{
	try
	{
		boost::asio::io_service service;
		Server server{service, "0.0.0.0", options.port};
		service.run();
	}
	catch (std::exception &e)
	{
		std::cerr << "An error occurred while running the server: " << e.what() << "\n";
	}
}

const char* const usage = "Usage: server PORT";
void printUsage() {
	std::cout << usage << std::endl;
}

Options getOptions(int argc, char *argv[])
{
	if (argc != 2) {
		printUsage();
		exit(EXIT_FAILURE);
	}

	Options options{8888};
	try{
		options.port = boost::lexical_cast<uint16_t>(argv[1]);
	}catch(boost::bad_lexical_cast&){
		printUsage();
		exit(EXIT_FAILURE);
	}

	return options;
}

int main(int argc, char *argv[])
{
	runServer(getOptions(argc, argv));

	std::cout << "Press enter to exit..." << std::endl;
	std::cin.clear();
	std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}
