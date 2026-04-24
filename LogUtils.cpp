#include "LogUtils.h"

void LogUtils::openLog(std::string logFileName) {
    std::lock_guard<std::mutex> lock(log_mutex);
    logFile.open(logFileName, std::ios::out | std::ios::trunc);

    if (!logFile.is_open()) {
        std::cerr << "Failed to open log file: " << logFileName << std::endl;
        exit(1);
    }
}

void LogUtils::closeLog() {
    std::lock_guard<std::mutex> lock(log_mutex);
    logFile.close();
}

void LogUtils::logMessage(std::string message) {
    logFile << message << std::endl;
}

void LogUtils::logTimestamp() {
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    logFile << "[";
    logFile << std::put_time(&tm, "%d-%m-%Y %H-%M-%S");
    logFile << "]: ";
}

void LogUtils::logConnectMake(unsigned int remote) {
    std::lock_guard<std::mutex> lock(log_mutex);
    logTimestamp();
    logFile << "Peer ";
    logFile << host;
    logFile << " makes a connection to Peer ";
    logFile << remote;
    logFile << ".";
    logFile << std::endl;
}

void LogUtils::logConnectRecv(unsigned int remote) {
    std::lock_guard<std::mutex> lock(log_mutex);
    logTimestamp();
    logFile << "Peer ";
    logFile << host;
    logFile << " is connected from Peer ";
    logFile << remote;
    logFile << ".";
    logFile << std::endl;
}

void LogUtils::logUpdatePrefNeighbors(const std::vector<unsigned int>& neighbors) {
    std::lock_guard<std::mutex> lock(log_mutex);
    logTimestamp();
    if (neighbors.size() == 0) {
        logFile << "Peer ";
        logFile << host;
        logFile << " has no preferred neighbors.";
        logFile << std::endl;
    } else {
        logFile << "Peer ";
        logFile << host;
        logFile << " has the preferred neighbors ";
        for (unsigned int i = 0; i < neighbors.size(); ++i) {
            if (i > 0) logFile << ", ";
            logFile << neighbors[i];
        }
        logFile << ".";
        logFile << std::endl;
    }
}

void LogUtils::logUpdateOptUnchokedNeighbor(unsigned int remote) {
    std::lock_guard<std::mutex> lock(log_mutex);
    logTimestamp();
    logFile << "Peer ";
    logFile << host;
    logFile << " has the optimistically unchoked neighbor ";
    logFile << remote;
    logFile << ".";
    logFile << std::endl;
}

void LogUtils::logUnchoking(unsigned int remote) {
    std::lock_guard<std::mutex> lock(log_mutex);
    logTimestamp();
    logFile << "Peer ";
    logFile << host;
    logFile << " is unchoked by Peer ";
    logFile << remote;
    logFile << ".";
    logFile << std::endl;
}

void LogUtils::logChoking(unsigned int remote) {
    std::lock_guard<std::mutex> lock(log_mutex);
    logTimestamp();
    logFile << "Peer ";
    logFile << host;
    logFile << " is choked by Peer ";
    logFile << remote;
    logFile << ".";
    logFile << std::endl;
}

void LogUtils::logRecvHave(unsigned int remote, unsigned int piece_id) {
    std::lock_guard<std::mutex> lock(log_mutex);
    logTimestamp();
    logFile << "Peer ";
    logFile << host;
    logFile << " received the 'have' message from ";
    logFile << remote;
    logFile << " for the piece ";
    logFile << piece_id;
    logFile << ".";
    logFile << std::endl;
}

void LogUtils::logRecvInterested(unsigned int remote) {
    std::lock_guard<std::mutex> lock(log_mutex);
    logTimestamp();
    logFile << "Peer ";
    logFile << host;
    logFile << " received the 'interested' message from ";
    logFile << remote;
    logFile << ".";
    logFile << std::endl;
}

void LogUtils::logRecvNotInterested(unsigned int remote) {
    std::lock_guard<std::mutex> lock(log_mutex);
    logTimestamp();
    logFile << "Peer ";
    logFile << host;
    logFile << " received the 'not interested' message from ";
    logFile << remote;
    logFile << ".";
    logFile << std::endl;
}

void LogUtils::logDownloaded(unsigned int remote, unsigned int piece, unsigned int total)  {
    std::lock_guard<std::mutex> lock(log_mutex);
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

void LogUtils::logCompletion()  {
    std::lock_guard<std::mutex> lock(log_mutex);
    logTimestamp();
    logFile << "Peer ";
    logFile << host;
    logFile << " has downloaded the complete file";
    logFile << std::endl;
}
