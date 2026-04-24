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
#include <vector>

struct PeerInfo {
    unsigned int id;
    std::string hostname;
    std::string port;
    bool hasCompleteFile;
} typedef PeerInfo;

class ConfigUtils {
	public:
		ConfigUtils() {
			readCommonConfig();
			readPeerInfoConfig();
		}

		PeerInfo* getPeer(int i);
		unsigned int getPeerIndex(unsigned int peerID);

		unsigned int getFileSize() const {
			return fileSize;
		}
		unsigned int getPieceSize() const {
			return pieceSize;
		}
		unsigned int getNumPieces() const {
			return (fileSize + pieceSize - 1) / pieceSize;
		}
		bool hasCompleteFile(unsigned int peer_id) const {
			return peerInfo[peerID2idx.at(peer_id)]->hasCompleteFile;
		}
		// void setComplete(unsigned int peer_id) {
		// 	peerInfo[peerID2idx[peer_id]]->hasCompleteFile = true;
		// }

	private:
		void readCommonConfig();
		void readPeerInfoConfig();

		/* Common.cfg vars */
		std::fstream common_config;
	    std::string common_filepath = "common.cfg";
		unsigned int numPreferredNeighbors = 0;
		unsigned int unchokingInterval = 999999;
		unsigned int optimisticUnchokingInterval = 999999; 
		unsigned int fileSize;
		unsigned int pieceSize;

		/* PeerInfo.cfg vars */
		std::vector<PeerInfo*> peerInfo;
		std::map<unsigned int, unsigned int> peerID2idx;
};