#include "includes.h"
#include "Server.h"
#include "Operations.h"

int main(int argc, char** argv)
{
	if (argc <= 1)
	{
		std::cout << "EXAMPLE USAGE:" << std::endl;
		std::cout << "--------------" << std::endl;
		std::cout << "HOST SERVER ONLY" << std::endl;
		std::cout << "\t" << argv[0] << " <PORT>" << std::endl;
		std::cout << "CONNECT TO SERVER ONLY" << std::endl;
		std::cout << "\t" << argv[0] << " <IP:PORT>" << std::endl;
		std::cout << "HOST SERVER + CONNECT TO SERVER" << std::endl;
		std::cout << "\t" << argv[0] << " <PORT> <IP:PORT> ..." << std::endl;
		exit(0);
	}

	std::vector<Server*> connections;

	for (int i = 1; i < argc; i++)
	{
		std::string arg(argv[i]);
		int delim;
		if ((delim = arg.find(":")) == std::string::npos)
			Server* server = new Server(atoi(argv[i]));
		else
		{
			std::string ip = arg.substr(0, delim);
			std::string port = arg.substr(delim + 1);
			connections.push_back(new Server(ip, atoi(port.c_str())));
		}
	}

	long long ping = Operations::getTimeMilli() + 5000 + 10000;

	while (true) {
		for (int i = 0; i < connections.size(); i++)
		{
			Server* server = connections[i];

			if (Operations::getTimeMilli() > ping)
			{
				printf("ERROR: LOST CONNECTION. Reconnecting...\n");
				server = new Server(server->getIP(), server->getPort());
				connections[i] = server;
				ping = Operations::getTimeMilli() + 5000 + 10000;
			}

			vector<string> messages = server->getMessages();
			for (string message : messages)
			{
				std::cout << "RECEIVED: " << message << std::endl;

				if (message == "PING")
				{
					int elapsed = Operations::getTimeMilli() - (ping - 10000);
					std::cout << "ELAPSED: " << elapsed << "ms" << std::endl;
					server->sendStr("PONG");
				}
				ping = Operations::getTimeMilli() + 5000 + 10000;
			}
		}
	}

	return 0;
}