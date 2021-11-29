#include <Server.h>
#include <RegisterHelper.h>
#include <Tools.h>
#include <iostream>

#if defined(_MSC_VER)
int gettimeofday(struct timeval* tp, struct timezone* tzp)
{
	// Note: some broken versions only have 8 trailing zero's, the correct epoch has 9 trailing zero's
	// This magic number is the number of 100 nanosecond intervals since January 1, 1601 (UTC)
	// until 00:00:00 January 1, 1970 
	static const uint64_t EPOCH = ((uint64_t)116444736000000000ULL);

	SYSTEMTIME  system_time;
	FILETIME    file_time;
	uint64_t    time;

	GetSystemTime(&system_time);
	SystemTimeToFileTime(&system_time, &file_time);
	time = ((uint64_t)file_time.dwLowDateTime);
	time += ((uint64_t)file_time.dwHighDateTime) << 32;

	tp->tv_sec = (long)((time - EPOCH) / 10000000L);
	tp->tv_usec = (long)(system_time.wMilliseconds * 1000);
	return 0;
}
#endif
long long getTimeMilli()
{
	timeval tv;
	gettimeofday(&tv, 0);
	return ((long long)tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}

class CheckRoundTime
{
public:
	CheckRoundTime()
	{
		time = getTimeMilli();
	}

	// 2. See goals below
	bool Ping(void* obj, std::vector<std::string> params, std::string body)
	{
		Connection* conn = (Connection*)obj;

		int ping = getTimeMilli() - time;
		this->avgPing = (this->avgPing == -1) ? ping : (this->avgPing + ping) / 2;

		// SEND: Ping
		conn->sendMessage(RegisteredKey(conn, this, Ping), { "a" }, "b");

		// 3. See goals below
		// Client doesn't have TestThroughput registered, so send TestThroughput to server but TestThroughputClient to client
		std::string key = RegisteredKey(conn, this, TestThroughput);
		if (key.length() == 0)
			key = RegisteredKey(conn, this, TestThroughputClient);

		std::string maxThroughputStr = "";
		for (int i = 0; i < conn->getMaxThroughput() - 4; i++) // Subtract 4 because that's how many digits are in the throughput size
			maxThroughputStr += "a";
		// SEND: (if server) TestThroughput (otherwise) TestThroughputClient.			See reason above, where key is being set
		conn->sendMessage(key, { "a" }, maxThroughputStr);

		time = getTimeMilli();

		return true;
	}

	bool TestThroughput(void* obj, std::vector<std::string> params, std::string body)
	{
		Client* client = (Client*)obj;

		// Do this once every e.g. 1,000 (maxThroughputTests) times
		throughputTests++;
		if (throughputTests > maxThroughputTests)
		{
			// Throughput defaults to 8192 at runtime. Set throughput to 1,000, then every 1,000 Pings increment throughput by another 1,000.
			int throughput = client->getMaxThroughput() == 8192 ? 1000 : client->getMaxThroughput() + 1000;
			client->requestSetMaxThroughput(throughput);

			std::cout << "THROUGHPUT WAS: " << client->getMaxThroughput() << ", CHANGING TO: " << throughput << std::endl;
			std::cout << "\tPING WAS (AVG): " << this->avgPing << std::endl;

			throughputTests = 0;
			this->avgPing = -1;
		}
		return true;
	}
	bool TestThroughputClient(void* obj, std::vector<std::string> params, std::string body)
	{
		return true; // Client does nothing, server does the testing
	}

	int time;
	int avgPing = -1;
	int throughputTests = 0, maxThroughputTests = 1000;
};

int main(int argc, char** argv)
{
	/** GOALS:
	 *		1. Start the client or server (server by default, client if passed ANY params).
	 *		2. When a new client connects to the server, the server starts the pinging process (where the client sends a ping back, continuously).
	 *		3. Each iteration, send a message that uses the entire throughput. After X amount of time (e.g. 10 times) change the throughput.
	 */

	// 1.
	if (argc == 1)
	{
		SocketServer* server = new SocketServer(2324);
		CheckRoundTime* sCrt = new CheckRoundTime();

		// 2.
		RegisterFunctionKeyLambda(server, onClientAccept, [], {
			Client * client = (Client*)obj;
			CheckRoundTime* cCrt = new CheckRoundTime();

			RegisterFunction(client, cCrt, Ping);
			RegisterFunction(client, cCrt, TestThroughput);

			std::string key = RegisteredKey(client, cCrt, Ping);
			client->sendMessage(key, { "a" }, "b");

			return true;
		});

		// Start the client process
		system("start /d \"C:\\Users\\treyt\\Desktop\\Homework\\Advanced Comm Networks\\Code\\Project1\\Debug\\\" Project1.exe TestClient");

		printf("----- SERVER -----\n");
		while (true)
		{
			server->Iterate();
		}
	}
	else
	{
		SocketClient* client = new SocketClient("192.168.56.1", 2324);
		CheckRoundTime* cCrt = new CheckRoundTime();

		// 2.
		RegisterFunction(client, cCrt, Ping);
		RegisterFunction(client, cCrt, TestThroughputClient);

		printf("----- CLIENT -----\n");
		while (true)
		{
			client->Iterate();
		}
	}

	return 0;
}