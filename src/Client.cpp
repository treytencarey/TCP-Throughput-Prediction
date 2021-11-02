#include "Client.h"
#include "Operations.h"
#include "Server.h"

Client::Client(int sock, int ID)
{
		this->sock = sock;
		this->ID = ID;
		this->sendPing(true,false);
}

Client::~Client()
{
	printf("Client %d disconnected.", this->ID);
	int pos;
	if ((pos = std::find(Server::clients.begin(), Server::clients.end(), this) - Server::clients.begin()) != Server::clients.size())
		Server::clients.erase(Server::clients.begin() + pos);
	closesocket(this->sock);
}

int pingTime = 5000;
int pongTime = 10000;
void Client::sendPing(bool setPong/*=true*/, bool sendToServer/*=true*/)
{
	this->ping = Operations::getTimeMilli() + pingTime;
	if (setPong)
		this->pong = this->ping + pongTime;
	if (sendToServer)
		this->sendMessage("PING");
}

void Client::getMessages()
{
	u_long iMode = 1;
	ioctlsocket(this->sock, FIONBIO, &iMode);

	char* msg = new char[255];

	int recvd = recv(this->sock, msg, 255, 0); //Server::recvAll(this->sock, &msg, 255, 0);
	if (recvd > 0)
	{
		std::string message(msg);
		std::cout << "RECEIVED: " << message << std::endl;

		if (message != "PONG")
		{
			for (Client* client : Server::clients)
			{
				if (client != this)
					client->sendMessage(msg);
			}
			this->sendPing(true, false);
		}
		else
		{
			int elapsed = Operations::getTimeMilli() - (this->pong - pongTime);
			std::cout << "ELAPSED: " << elapsed << "ms" << std::endl;
			this->sendPing(true, false);
		}
		
	}

	delete msg;
}

void Client::sendMessages()
{
	u_long iMode = 1;
	ioctlsocket(this->sock, FIONBIO, &iMode);

	auto time = Operations::getTimeMilli();
	// std::cout << time << ", " << this->ping << ", " << this->pong << std::endl;
	if (this->pong < time)
	{
		delete this;
		return;
	}
	if (this->ping < time)
		this->sendPing(false);

	for (string message : this->messages)
	{
		std::cout << "SENDING: " << message << std::endl;
		char* msg = new char[255];

		memcpy(msg, message.data(), 255);
		int sent = send(this->sock, msg, 255, 0);

		delete msg;
	}
	this->messages.clear();
}

void Client::sendMessage(string msg)
{
	this->messages.push_back(msg);
}