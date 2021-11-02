#pragma once

#include "includes.h"
#include "Client.h"

class Client;

class Server {
public:
	static vector<Client*> clients;

	Server(string ip, int port);
	Server(int port);

	void sendStr(string str);
	vector<string> getMessages();

	static void Connection(int threadID);
	int sendAll(int sockfd, const void* buf, size_t len, int flags);
	int recvAll(int sockfd, const void* buf, size_t len, int flags);

	std::string getIP();
	int getPort();

private:
	bool online = false;
	int sock;
	std::string IP;
	int port;
};