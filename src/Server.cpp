#include "Server.h"
#include "Client.h"
#include "includes.h"

vector<Client*> Server::clients;

Server::Server(int port)
{
	this->port = port;
	/// Start the socket server
	int server_skt, client_skt;

	WSADATA wsaData;
	WSAStartup(0x101, &wsaData);

	sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;//IPv4
	server_addr.sin_port = htons(port);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	memset(server_addr.sin_zero, 0, 8);

	if ((server_skt = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		// printf("%s", strerror(errno));
		exit(14);
	}

	if (bind(server_skt, (struct sockaddr*) & server_addr, sizeof(struct sockaddr)) == -1) {
		exit(13);
	}

	int queueSize = 1;
	if (listen(server_skt, queueSize) == -1) {
		exit(12);
	}

	printf("Waiting for a connection.\n");
	int ID = 0;
	bool running = true;
	while (running) {
		int sin_size = sizeof(struct sockaddr_in);
		int temp = sin_size;

		sockaddr clients_addr;
		if ((client_skt = accept(server_skt, &clients_addr, &temp)) != -1) {
			Client* newClient = new Client(client_skt, ID);
			if (ID == 0)
				std::thread(Server::Connection, ID).detach();
			printf("Client %d connected!\n", ID);
			Server::clients.push_back(newClient);
			ID++;
		}
	}
}

Server::Server(string ip, int port)
{
	this->IP = ip;
	this->port = port;
	sockaddr_in ser;
	sockaddr addr;

	WSADATA WSdata;
	if (WSAStartup(MAKEWORD(1, 1), &WSdata) != 0) {
		exit(255);
	}

	ser.sin_family = AF_INET;
	ser.sin_port = htons(port);
	ser.sin_addr.s_addr = inet_addr(ip.c_str());

	memcpy(&addr, &ser, 16);

	int res = 0;
	res = WSAStartup(MAKEWORD(1, 1), &WSdata);      //Start Winsock

	if (res != 0)
		exit(0);

	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);       //Create the socket

	fd_set fd;

	bool cont = true;
	while (cont)
	{
		FD_ZERO(&fd);
		FD_SET(sock, &fd);

		u_long iMode = 1;
		ioctlsocket(sock, FIONBIO, &iMode);

		res = connect(sock, &addr, sizeof(addr));

		struct timeval timeout;
		timeout.tv_sec = 5;
		timeout.tv_usec = 0;

		cont = false;
		/* Wait for write bit to be set */
		if (select(FD_SETSIZE, 0, &fd, 0, &timeout) <= 0)
		{
			this->online = true;
			cont = true;
			// cout << "Connection Failed. Try again." << endl;
			// system("pause");
			// exit(0);
		}
	}
	cout << "Connection success!" << endl;
}

std::string Server::getIP()
{
	return this->IP;
}

int Server::getPort()
{
	return this->port;
}

void Server::Connection(int threadID)
{
	bool running = true;
	while (running)
	{
		for (int ID = Server::clients.size() - 1; ID >= 0; ID--) {
			Client* client = Server::clients[ID];
			
			client->getMessages();
			client->sendMessages();
		}
	}
}

int Server::recvAll(int sockfd, const void* buf, size_t len, int flags) {
	u_long iMode = 1;
	ioctlsocket(sockfd, FIONBIO, &iMode);

	int result;
	int recvd = 0;
	char* pbuf = (char*)buf;
	while (len > 0) {
		result = recv(sockfd, pbuf, len, flags);

		if (result <= 0) break;

		recvd += result;
		pbuf += result;
		len -= result;
	}

	return recvd;
}

vector<string> Server::getMessages()
{
	u_long iMode = 1;
	ioctlsocket(this->sock, FIONBIO, &iMode);

	char* msg = new char[255];

	vector<string> messages;

	int recvd;
	while ((recvd = recv(this->sock, msg, 255, 0)) == 255)
	{
		messages.push_back(msg);
	}

	delete msg;
	return messages;
}

void Server::sendStr(string str)
{
	u_long iMode = 1;
	ioctlsocket(this->sock, FIONBIO, &iMode);

	str += "\0";
	char msg[255];
	strcpy_s(msg, str.c_str());

	std::cout << "SENDING: " << msg << std::endl;
	int ao = send(this->sock, msg, 255, 0);
}

int Server::sendAll(int sockfd, const void* buf, size_t len, int flags) {
	if (!Server::online)
		return len;
	int result;
	char* pbuf = (char*)buf;
	while (len > 0) {
		result = send(sockfd, pbuf, len, flags);
		if (result <= 0) break;
		pbuf += result;
		len -= result;
	}
	return result;
}