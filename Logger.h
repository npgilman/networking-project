

public class Logger {

	public:
		Logger(int id, std::string filename) {
			this->host = id;
			openLog(filename);
		}

		void openLog(std::string logFileName);
		void closeLog();
		void logMessage(std::string msg);
		void logTimestamp();
		void logConnectMake(unsigned int remote);
		void logConnectRecv(unsigned int remote);
		void logUpdatePrefNeighbors(unsigned int* remoteArr);
		void logUpdateOptUnchokedNeighbor(unsigned int remote);
		void logUnchoking(unsigned int remote);
		void logChoking(unsigned int remote);
		void logRecvHave(unsigned int remote);
		void logRecvInterested(unsigned int remote);
		void logRecvNotInterested(unsigned int remote);
		void logDownloaded(unsigned int remote, unsigned int piece, unsigned int total);
		void logCompletion();

	private:
		std::fstream logFile;
		int host;
}