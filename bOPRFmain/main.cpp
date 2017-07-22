#include <iostream>

using namespace std;
#include "Common/Defines.h"
#include "PSI/BopPsiReceiver.h"
#include "PSI/BopPsiSender.h"
#include "Network/BtEndpoint.h" 
#include <math.h>
#include "Common/Log.h"
#include "Common/Timer.h"
#include "Crypto/PRNG.h"
using namespace bOPRF;
#include <fstream>



void senderGetLatency(Channel& chl)
{
	u8 dummy[1];
	chl.asyncSend(dummy, 1);
	chl.recv(dummy, 1);
	chl.asyncSend(dummy, 1);
}

void recverGetLatency(Channel& chl)
{
	u8 dummy[1];
	chl.recv(dummy, 1);
	Timer timer;
	auto start = timer.setTimePoint("");
	chl.asyncSend(dummy, 1);
	chl.recv(dummy, 1);
	auto end = timer.setTimePoint("");
	std::cout << "latency: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms" << std::endl;
}

void pingTest(Channel& chl, bool sender)
{
	u64 count = 100;
	std::array<u8, 131072 / 100> oneMB;

	Timer timer;
	ByteStream buff;
	if (sender)
	{
		auto send = timer.setTimePoint("ping sent");
		for (u64 i = 0; i < count; ++i)
		{
			chl.asyncSend("c", 1);
			chl.recv(buff);
			if (buff.size() != 1)
			{
				std::cout << std::string((char*)buff.data(), (char*)buff.data() + buff.size()) << std::endl;
				throw std::runtime_error("");
			}
		}
		chl.asyncSend("r", 1);

		auto recv = timer.setTimePoint("ping recv");

		auto ping = std::chrono::duration_cast<std::chrono::microseconds>(recv - send).count() / count;

		std::cout << "ping " << ping << " us" << std::endl;

		send = timer.setTimePoint("");
		chl.asyncSend(oneMB.data(), oneMB.size());
		chl.recv(buff);
		recv = timer.setTimePoint("");
		if (buff.size() != 1) throw std::runtime_error("");

		double time = static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(recv - send).count() - ping);

		chl.recv(buff);
		chl.asyncSend("r", 1);
		if (buff.size() != oneMB.size()) throw std::runtime_error("");


		std::cout << (8000000 / time) << " Mbps" << std::endl;
	}
	else
	{
		chl.recv(buff);

		auto send = timer.setTimePoint("ping sent");
		for (u64 i = 0; i < count; ++i)
		{
			chl.asyncSend("r", 1);
			chl.recv(buff);
			if (buff.size() != 1) throw std::runtime_error("");

		}

		auto recv = timer.setTimePoint("ping recv");

		auto ping = std::chrono::duration_cast<std::chrono::microseconds>(recv - send).count() / count;
		std::cout << "ping " << ping << " us" << std::endl;

		chl.recv(buff);
		chl.asyncSend("r", 1);
		if (buff.size() != oneMB.size()) throw std::runtime_error("");


		send = timer.setTimePoint("");
		chl.asyncSend(oneMB.data(), oneMB.size());
		chl.recv(buff);
		recv = timer.setTimePoint("");
		if (buff.size() != 1) throw std::runtime_error("");

		double time = static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(recv - send).count() - ping);

		std::cout << (8000000 / time) << " Mbps" << std::endl;

	}

}

void BopSender()
{
	std::cout << "BopSender()" << std::endl;
	u64 numThreads = 1;
	u64 numTrial(10);

	std::fstream online, offline;

	std::cout << "role  = sender (" << numThreads << ") SSOtPSI" << std::endl;

	// , numThreads(1);


	std::string name("psi");

	BtIOService ios(0);
	BtEndpoint ep0(ios, "localhost", 1215, true, name);


	std::vector<Channel*> sendChls(numThreads);
	for (u64 i = 0; i < numThreads; ++i)
		sendChls[i] = &ep0.addChannel(name + std::to_string(i), name + std::to_string(i));

	senderGetLatency(*sendChls[0]);

	pingTest(*sendChls[0], true);


	for (u64 pow : { 16,20,24 })
	{

		for (u64 recverSize : { 5535, 11041 })
		{
			u64 senderSize = (1 << pow), psiSecParam = 40;
			recverSize = recverSize ? recverSize : senderSize;

			u64 offlineTimeTot(0);
			u64 onlineTimeTot(0);

			for (u64 j = 0; j < numTrial; ++j)
			{
				//u64 repeatCount = 4;
				PRNG prngSame(_mm_set_epi32(4253465, 3434565, 234435, 23987045));
				//	PRNG prngSame2(_mm_set_epi32(4253465, 3434565, 234435, 23987045));
				PRNG prngDiff(_mm_set_epi32(43465, 32254, 2435, 2398045));

				std::vector<block> sendSet(senderSize);

				u64 rand = prngSame.get_u32() % std::min(senderSize, recverSize);

				for (u64 i = 0; i < rand; ++i)
				{
					sendSet[i] = prngSame.get_block();
				}
				for (u64 i = rand; i < senderSize; ++i)
				{
					sendSet[i] = prngDiff.get_block();
				}
				//std::shuffle(sendSet.begin(), sendSet.end(), prngSame);
				SSOtExtSender OTSender0;


				BopPsiSender sendPSIs;

				u8 dumm[1];
				sendChls[0]->asyncSend(dumm, 1);
				sendChls[0]->recv(dumm, 1);
				sendChls[0]->asyncSend(dumm, 1);


				gTimer.reset();
				sendPSIs.init(senderSize, recverSize, psiSecParam, *sendChls[0], OTSender0, OneBlock);


				sendPSIs.sendInput(sendSet, sendChls);
				//std::cout << "threads =  " << numThreads << std::endl << gTimer << std::endl << std::endl << std::endl;
				std::cout << "sent " << sendChls[0]->getTotalDataSent() << std::endl;;

				u64 otIdx = 0;
			}
		}
	}

	for (u64 i = 0; i < numThreads; ++i)
	{
		sendChls[i]->close();
	}
	//sendChl.close();
	//recvChl.close();

	ep0.stop();
	ios.stop();
}

void BopRecv()
{
	std::cout << "BopRecv()" << std::endl;

	u64 numThreads = 1;
	//u64 repeatCount = 4;


	std::fstream online, offline, total;
	total.open("./output.txt", total.trunc | total.out);
	u64 numTrial(10);

	std::string name("psi");

	BtIOService ios(0);
	BtEndpoint ep1(ios, "localhost", 1215, false, name);


	std::vector<Channel*> recvChls(numThreads);
	for (u64 i = 0; i < numThreads; ++i)
		recvChls[i] = &ep1.addChannel(name + std::to_string(i), name + std::to_string(i));

	recverGetLatency(*recvChls[0]);
	pingTest(*recvChls[0], false);
	std::cout << "role  = recv (" << numThreads << ") SSOtPSI" << std::endl;
	//8,12,16,
	std::cout << "--------------------------\n";


	for (u64 pow : { 16,20,24 })
	{
		for (u64 recverSize : {  5535, 11041 })
		{
			u64 sendSize = (1 << pow), psiSecParam = 40;
			recverSize = recverSize ? recverSize : sendSize;


			u64 offlineTimeTot(0);
			u64 onlineTimeTot(0);

			std::cout << "setSize" << "       |  " << "offline(ms)" << "  |  " << "online(ms)" << std::endl;

			for (u64 j = 0; j < numTrial; ++j)
			{
				PRNG prngSame(_mm_set_epi32(4253465, 3434565, 234435, 23987045));
				PRNG prngDiff(_mm_set_epi32(434653, 23, 11, 56));

				std::vector<block> recvSet(recverSize);
				u64 rand = prngSame.get_u32() % std::min(sendSize, recverSize);
				for (u64 i = 0; i < rand; ++i)
				{
					recvSet[i] = prngSame.get_block();
				}

				for (u64 i = rand; i < recverSize; ++i)
				{
					recvSet[i] = prngDiff.get_block();
				}

				SSOtExtReceiver OTRecver0;
				BopPsiReceiver recvPSIs;

				u8 dumm[1];
				recvChls[0]->recv(dumm, 1);
				recvChls[0]->asyncSend(dumm, 1);
				recvChls[0]->recv(dumm, 1);

				//gTimer.reset();
				Timer timer;
				timer.setTimePoint("start");
				auto start = timer.setTimePoint("start");
				recvPSIs.init(sendSize, recverSize, psiSecParam, recvChls, OTRecver0, ZeroBlock);
				auto mid = timer.setTimePoint("init");
				recvPSIs.sendInput(recvSet, recvChls);
				//timer.setTimePoint("Done");
				auto end = timer.setTimePoint("done");

				auto offlineTime = std::chrono::duration_cast<std::chrono::milliseconds>(mid - start).count();
				auto online = std::chrono::duration_cast<std::chrono::milliseconds>(end - mid).count();
				offlineTimeTot += offlineTime;
				onlineTimeTot += online;

				std::cout << "sent " << recvChls[0]->getTotalDataSent() << std::endl;;

				//output
				//std::cout << "#Output Intersection: " << recvPSIs.mIntersection.size() << std::endl;
				//std::cout << "#Expected Intersection: " << rand << std::endl;
				std::cout << recverSize << " vs " << sendSize << "\t\t" << offlineTime << "\t\t" << online << std::endl;

			}
			std::cout << recverSize << " vs 2^" << pow << "-- Online Avg Time: " << onlineTimeTot / numTrial << " ms " << "\n";
			std::cout << recverSize << " vs 2^" << pow << "-- Offline Avg Time: " << offlineTimeTot / numTrial << " ms " << "\n";
			std::cout << recverSize << " vs 2^" << pow << "-- Total Avg Time: " << (offlineTimeTot + onlineTimeTot) / numTrial << " ms " << "\n";
			std::cout << "--------------------------\n";

			total << "2^" << pow << "-- Online Avg Time: " << onlineTimeTot / numTrial << " ms " << "\n";
			total << "2^" << pow << "-- Offline Avg Time: " << offlineTimeTot / numTrial << " ms " << "\n";
			total << "2^" << pow << "-- Total Avg Time: " << (offlineTimeTot + onlineTimeTot) / numTrial << " ms " << "\n";
			total << "--------------------------\n";

		}

	}

	for (u64 i = 0; i < numThreads; ++i)
	{
		recvChls[i]->close();
	}
	//sendChl.close();
	//recvChl.close();

	ep1.stop();
	ios.stop();
}


void BopTest()
{
	std::cout << "Test()" << std::endl;

	u64 numThreads = 1;
	u64 senderSize = (1 << 12), psiSecParam = 40;// , numThreads(1);
	u64 recverSize = (1 << 12);// , numThreads(1);

	//generate data
	PRNG prng(_mm_set_epi32(4253465, 3434565, 234435, 23987045));
	PRNG prngDiff1(_mm_set_epi32(434653, 23, 11, 56));
	PRNG prngDiff2(_mm_set_epi32(43465, 32254, 2435, 2398045));

	std::vector<block> sendSet(senderSize), recvSet(recverSize);

	u64 rand = std::min(senderSize, recverSize) / 4;
	//same input value =>intersection
	for (u64 i = 0; i < rand; ++i)
	{
		sendSet[i] = prng.get_block();
		recvSet[i] = sendSet[i];
	}

	//different input value 
	for (u64 i = rand; i < senderSize; ++i)
	{
		sendSet[i] = prngDiff1.get_block();
	}
	for (u64 i = rand; i < recverSize; ++i)
	{
		recvSet[i] = prngDiff2.get_block();
	}

	//shuffle
	std::shuffle(sendSet.begin(), sendSet.end(), prng);
	std::shuffle(recvSet.begin(), recvSet.end(), prng);


	std::string name("psi");

	BtIOService ios(0);
	BtEndpoint ep0(ios, "localhost", 1213, true, name);
	BtEndpoint ep1(ios, "localhost", 1213, false, name);


	std::vector<Channel*> recvChls(numThreads), sendChls(numThreads);
	for (u64 i = 0; i < numThreads; ++i)
		recvChls[i] = &ep1.addChannel(name + std::to_string(i), name + std::to_string(i));
	for (u64 i = 0; i < numThreads; ++i)
		sendChls[i] = &ep0.addChannel(name + std::to_string(i), name + std::to_string(i));


	SSOtExtSender OTSender0;
	SSOtExtReceiver OTRecver0;

	auto bb = prng.get_block();

	BopPsiSender sendPSIs;
	BopPsiReceiver recvPSIs;

	std::thread thrd([&]() {
		PRNG prng(bb);
		//sender thread
		sendPSIs.init(senderSize, recverSize, psiSecParam, *sendChls[0], OTSender0, OneBlock);
		sendPSIs.sendInput(sendSet, sendChls);
	});

	u64 otIdx = 0;

	gTimer.reset();
	//receiver thread
	auto start = gTimer.setTimePoint("start");
	recvPSIs.init(senderSize, recverSize, psiSecParam, recvChls, OTRecver0, ZeroBlock);
	recvPSIs.sendInput(recvSet, recvChls);
	auto end = gTimer.setTimePoint("done");


	auto totalTime = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
	std::cout << gTimer << std::endl;

	std::cout << "Total Time: " << totalTime << "(ms)\t\t\n" << std::endl;

	//output
	std::cout << "#Output Intersection: " << recvPSIs.mIntersection.size() << std::endl;
	std::cout << "#Expected Intersection: " << rand << std::endl;

	if (recvPSIs.mIntersection.size() != rand)
	{
		std::cout << "\nbad intersection,  expecting full set of size  " << senderSize << " but got " << recvPSIs.mIntersection.size() << std::endl;
		//throw std::runtime_error(std::string("bad intersection, "));
	}
	else
	{
		std::cout << "\nCool. Good PSI!" << std::endl;
	}

	thrd.join();

	for (u64 i = 0; i < numThreads; ++i)
	{
		sendChls[i]->close();
		recvChls[i]->close();
	}

	ep0.stop();
	ep1.stop();
	ios.stop();


}

void usage(const char* argv0)
{
	std::cout << "Error! Please use:" << std::endl;
	std::cout << "\t 1. For unit test: " << argv0 << " -t" << std::endl;
	std::cout << "\t 2. For simulation (2 terminal): " << std::endl;;
	std::cout << "\t\t Sender terminal: " << argv0 << " -r 0" << std::endl;
	std::cout << "\t\t Receiver terminal: " << argv0 << " -r 1" << std::endl;
}

int main(int argc, char** argv)
{
	BopTest();
	return 0;

	//for (auto p : { 12, 16, 20, 24, 28 })
	//{
	//	Timer t;
	//	auto s = t.setTimePoint("");
	//	auto B = get_bin_size(1 << 13, 3 *( u64(1) << p), 40);
	//	auto e = t.setTimePoint("");
	//	std::cout << 1 << 13 << " p " << p << "   B " << B << " "<< std::chrono::duration_cast<std::chrono::milliseconds>(e-s).count() << std::endl;
	//}

	if (argc == 2 && argv[1][0] == '-' && argv[1][1] == 't') {
		BopTest();
	}
	else if (argc == 3 && argv[1][0] == '-' && argv[1][1] == 'r' && atoi(argv[2]) == 0) {
		BopSender();
	}
	else if (argc == 3 && argv[1][0] == '-' && argv[1][1] == 'r' && atoi(argv[2]) == 1) {
		BopRecv();
	}
	else {
		usage(argv[0]);
	}

	return 0;
}