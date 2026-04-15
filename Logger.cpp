#include "Logger.h"

void Logger::openLog(std::string logFileName) {
    logFile.open(logFileName, std::ios::out | std::ios::trunc);

    if (!logFile.is_open()) {
        std::cerr << "Failed to open log file: " << logFileName << std::endl;
        exit(1);
    }
}

void Logger::closeLog() {
    logFile.close();
}

void Logger::logMessage(std::string message) {
    logFile << message << std::endl;
}

void Logger::logTimestamp() {
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    logFile << "[";
    logFile << std::put_time(&tm, "%d-%m-%Y %H-%M-%S");
    logFile << "]: ";
}

void Logger::logConnectMake(unsigned int remote) {
    logTimestamp();
    logFile << "Peer ";
    logFile << host;
    logFile << " makes a connection to Peer ";
    logFile << remote;
    logFile << ".";
    logFile << std::endl;
}

void Logger::logConnectRecv(unsigned int remote) {
    logTimestamp();
    logFile << "Peer ";
    logFile << host;
    logFile << " is connected from Peer ";
    logFile << remote;
    logFile << ".";
    logFile << std::endl;
}

void Logger::logUpdatePrefNeighbors(unsigned int* remoteArr) {
    logTimestamp();
    if (remoteArr == nullptr) {} // TODO: print a line and exit

    logFile << "Peer ";
    logFile << host;
    logFile << "has the preferred neighbors ";
    // logFile << remoteArr; // TODO: print the array of neighbors
    logFile << ".";
    logFile << std::endl;
}

void Logger::logUpdateOptUnchokedNeighbor(unsigned int remote) {
    logTimestamp();
    logFile << "Peer ";
    logFile << host;
    logFile << " has the optimistically unchoked neighbor ";
    logFile << remote;
    logFile << ".";
    logFile << std::endl;
}

void Logger::logUnchoking(unsigned int remote) {
    logTimestamp();
    logFile << "Peer ";
    logFile << host;
    logFile << " is unchoked by Peer ";
    logFile << remote;
    logFile << ".";
    logFile << std::endl;
}

void Logger::logChoking(unsigned int remote) {
    logTimestamp();
    logFile << "Peer ";
    logFile << host;
    logFile << " is choked by Peer ";
    logFile << remote;
    logFile << ".";
    logFile << std::endl;
}

void Logger::logRecvHave(unsigned int remote) {
    logTimestamp();
    logFile << "Peer ";
    logFile << host;
    logFile << " received the 'have' message from ";
    logFile << remote;
    logFile << ".";
    logFile << std::endl;
}

void Logger::logRecvInterested(unsigned int remote) {
    logTimestamp();
    logFile << "Peer ";
    logFile << host;
    logFile << " received the 'interested' message from ";
    logFile << remote;
    logFile << ".";
    logFile << std::endl;
}

void Logger::logRecvNotInterested(unsigned int remote) {
    logTimestamp();
    logFile << "Peer ";
    logFile << host;
    logFile << " received the 'not interested' message from ";
    logFile << remote;
    logFile << ".";
    logFile << std::endl;
}

void Logger::logDownloaded(unsigned int remote, unsigned int piece, unsigned int total)  {
    logTimestamp();
    logFile << "Peer ";
    logFile << host;
    logFile << " has downloaded the piece ";
    logFile << piece;
    logFile << " from ";
    logFile << remote;
    logFile << ". Now the number of pieces it has is ";
    logFile << total;
    logFile << ".";
    logFile << std::endl;
}

void Logger::logCompletion()  {
    logTimestamp();
    logFile << "Peer ";
    logFile << host;
    logFile << " has downloaded the complete file";
    logFile << std::endl;
}
