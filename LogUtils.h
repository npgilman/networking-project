#pragma once

#include <ctime>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <map>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <unistd.h>
#include <mutex>

class LogUtils {
	public:
		LogUtils(int id, std::string filename) {
			this->host = id;
			openLog(filename);
		}

		void openLog(std::string logFileName);
		void closeLog();
		void logConnectMake(unsigned int remote);
		void logConnectRecv(unsigned int remote);
		void logUpdatePrefNeighbors(const std::vector<unsigned int>& neighbors);
		void logUpdateOptUnchokedNeighbor(unsigned int remote);
		void logUnchoking(unsigned int remote);
		void logChoking(unsigned int remote);
		void logRecvHave(unsigned int remote, unsigned int piece_id);
		void logRecvInterested(unsigned int remote);
		void logRecvNotInterested(unsigned int remote);
		void logDownloaded(unsigned int remote, unsigned int piece, unsigned int total);
		void logCompletion();

	private:
		void logMessage(std::string msg);
		void logTimestamp();
		std::mutex log_mutex;
		std::fstream logFile;
		int host;
};