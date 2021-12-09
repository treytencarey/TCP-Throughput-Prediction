#include <Server.h>
#include <RegisterHelper.h>
#include <Tools.h>
#include <iostream>

#include <preprocessing.h>
#include <lsr.h>
class Prediction
{
public:
	static std::vector<double> data1;
	static std::vector<double> data2;
	static simple_linear_regression* slr;

	/**
	 * \brief Adds data to the prediction model.
	 * 
	 * \param The first (independent) variable.
	 * \param The second (dependent) variable.
	 */
	static void addData(double val1, double val2)
	{
		Prediction::data1.push_back(val1);
		Prediction::data2.push_back(val2);
	}

	/*
	 * \brief "Crunches" the numbers based on the prediction model connection.
	 */
	static void crunch()
	{
		Prediction::slr = new simple_linear_regression(data1, data2, DEBUG);
	}

	/*
	 * \brief Predicts the dependent value, given the independent value.
	 * 
	 * \param The first (independent) variable.
	 */
	static double predict(double val1)
	{
		return Prediction::slr->predict(val1);
	}
};
std::vector<double> Prediction::data1;
std::vector<double> Prediction::data2;
simple_linear_regression* Prediction::slr;

class CheckRoundTime
{
public:
	CheckRoundTime()
	{
		time = Tools::getTimeMilli();
	}

	// 4. See goals below
	bool SetTraining(void* obj, std::vector<std::string> params, std::string body)
	{
		this->isTraining = body == "1";
		std::cout << "Connected to server as a " << std::string((this->isTraining) ? "TRAINING MODEL" : "CLIENT") << std::endl;

		return true;
	}

	// 2. See goals below
	bool Ping(void* obj, std::vector<std::string> params, std::string body)
	{
		Connection* conn = (Connection*)obj;

		int ping = Tools::getTimeMilli() - time;
		this->avgPing = (this->avgPing == -1) ? ping : (this->avgPing + ping) / 2;

		// SEND: Ping
		conn->sendMessage(RegisteredKey(conn, this, Ping), { "a" }, "b");

		// 3. See goals below
		// Client doesn't have TestThroughputServer registered, so send TestThroughputServer to server but TestThroughputClient to client
		std::string key = RegisteredKey(conn, this, TestThroughputServer);
		if (key.length() == 0)
			key = RegisteredKey(conn, this, TestThroughputClient);

		std::string maxThroughputStr = "";
		for (int i = 0; i < conn->getMaxThroughput() - 4; i++) // Subtract 4 because that's how many digits are in the throughput size
			maxThroughputStr += "a";
		// SEND: (if server) TestThroughputServer (otherwise) TestThroughputClient.			See reason above, where key is being set
		conn->sendMessage(key, { "a" }, maxThroughputStr);

		time = Tools::getTimeMilli();

		return true;
	}

	bool TestThroughputServer(void* obj, std::vector<std::string> params, std::string body)
	{
		Connection* conn = (Connection*)obj;

		// Do this once every e.g. 1,000 (maxThroughputTests) times
		throughputTests++;
		if (throughputTests > maxThroughputTests)
		{
			// Throughput defaults to 8192 at runtime. Set throughput to 1,000, then every 1,000 Pings increment throughput by another 5,000.
			int throughput = conn->getMaxThroughput() == 8192 ? 1000 : conn->getMaxThroughput() + 5000;
			conn->requestSetMaxThroughput(throughput);

			Prediction::addData(conn->getMaxThroughput(), this->avgPing);
			std::cout << "THROUGHPUT WAS: " << conn->getMaxThroughput() << ", CHANGING TO: " << throughput << std::endl;
			std::cout << "\tPING WAS (AVG): " << this->avgPing << std::endl;
			// 5. See goals below
			if (!this->isTraining)
			{
				std::cout << "\tPREDICTED PING WAS: " << Prediction::predict(conn->getMaxThroughput()) << std::endl;
			}

			throughputTests = 0;
			this->avgPing = -1;
		}
		return true;
	}
	bool TestThroughputClient(void* obj, std::vector<std::string> params, std::string body)
	{
		this->totalTests--; // Used for the training model
		return true;
	}

	int time;
	int avgPing = -1;
	int throughputTests = 0, maxThroughputTests = 1500;
	
	bool isTraining = false;
	int totalTests = 10 * maxThroughputTests;
};

int main(int argc, char** argv)
{
	/** GOALS:
	 *		1. Start the client or server (server by default, client if passed ANY params).
	 *		2. When a new client connects to the server, the server starts the pinging process (where the client sends a ping back, continuously).
	 *		3. Each iteration, send a message that uses the entire throughput. After X amount of time (e.g. 10 times) change the throughput.
	 *		4. The first client to connect is the training model (for now). It looks at the amount of time in milliseconds it takes to send some number of bytes, and adds it to the training data.
	 *		5. Additional clients connect as regular clients. During the pinging process, it uses the training data from the first client to predict the amount of time it should have taken for some number of bytes to send.
	 */

	// 1.
	if (argc == 1)
	{
		SocketServer* server = new SocketServer(2324);
		CheckRoundTime* sCrt = new CheckRoundTime();

		bool doneTraining = false;

		// 2.
		RegisterFunctionKeyLambdaByName(server, onClientAccept, [&doneTraining],{
			std::string key;

			std::cout << std::string((!doneTraining) ? "TRAINING MODEL" : "CLIENT") << " connected." << std::endl;
			
			Client * client = (Client*)obj;
			CheckRoundTime* cCrt = new CheckRoundTime();

			RegisterFunction(client, cCrt, SetTraining);
			RegisterFunction(client, cCrt, Ping);
			RegisterFunction(client, cCrt, TestThroughputServer);

			// 4.
			if (!doneTraining)
			{
				cCrt->isTraining = true;
				key = RegisteredKey(client, cCrt, SetTraining);
				client->sendMessage(key, { "a" }, "1");
				doneTraining = true;

				// Training model is complete, crunch the numbers and wait for another connection.
				RegisterFunctionKeyLambdaByName(client, onDisconnect, [],{
					Prediction::crunch();

					std::cout << "TRAINING MODEL ENDED." << std::endl;
					std::cout << "\tNext you'll need to start the model to test predictions against." << std::endl;
					std::cout << "\tStart a client by passing an IP address to continue." << std::endl;
				
					return true;
				});
			}
			key = RegisteredKey(client, cCrt, Ping);
			client->sendMessage(key, { "a" }, "b");

			return true;
		});

		// Start the client process
		// system("start /d \"C:\\Users\\treyt\\Desktop\\Homework\\Advanced Comm Networks\\Code\\Project1\\Debug\\\" Project1.exe TestClient");
		std::cout << "\tFirst you'll need to start the training model." << std::endl;
		std::cout << "\tStart a client by passing an IP address to continue." << std::endl;

		printf("----- SERVER -----\n");
		while (true)
		{
			server->Iterate();
		}
	}
	else
	{
		SocketClient* client = new SocketClient(argv[1], 2324);
		CheckRoundTime* cCrt = new CheckRoundTime();

		// 2.
		RegisterFunction(client, cCrt, SetTraining);
		RegisterFunction(client, cCrt, Ping);
		RegisterFunction(client, cCrt, TestThroughputClient);

		printf("----- CLIENT -----\n");
		while (true)
		{
			client->Iterate();
			if (cCrt->isTraining && cCrt->totalTests < 0) // If we're a training model and all tests are complete, exit
				break;
		}
	}

	return 0;
}