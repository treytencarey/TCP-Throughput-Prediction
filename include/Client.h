#pragma once

#include "includes.h"

class Client
{
public:
	Client(int sock, int ID);
	~Client();

	void getMessages();
	void sendMessages();
	void sendMessage(string msg);
	void sendPing(bool setPong=true, bool sendToServer=true);

	long long ping = 0;
	long long pong = 0;
private:
	int sock, ID;
	vector<string> messages;
};