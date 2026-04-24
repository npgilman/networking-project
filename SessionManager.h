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
#include <iostream>
#include <cstring>
#include <set>
#include <random>

#include "ConfigUtils.h"
#include "ConnectionManager.h"

struct PeerState {
    std::vector<uint8_t> bitfield;
    bool interested = false;
    bool choked = false;
    bool remote_interested = false;
    bool remote_choked = true;
    float downloadRate = 0.0;
    ConnectionManager* conn = nullptr;
} typedef PeerState;

class SessionManager {
	public: 
		SessionManager(unsigned int PeerID, const ConfigUtils& config) : configUtils(config) {
			peerID = PeerID;
			initBitfield();
			openFile();
		}
		~SessionManager() {
			if (fileObj.is_open()) {
				fileObj.close();
			}
		}

		/* communication functions */
		bool hasPiece(uint32_t pieceIndex);
		unsigned int actualPieceSize(unsigned int index);
		bool hasCompleteFile();
		bool hasAnyPieces();
		bool isInteresting(std::vector<uint8_t>& neighbor_bitfield);
		unsigned int pieceSize(unsigned int index);
		std::vector<uint8_t> myBitfield();
		bool allPeersComplete();
		unsigned int countPieces();

		/* initiallization functions */
		void addNeighbor(unsigned int neighbor_id, const std::vector<uint8_t>& neighbor_bitfield);
		void setNeighborConn(unsigned int neighbor_id, ConnectionManager* conn);


		/* toggle interests */
		void setInterested(unsigned int neighbor_id);
		void setUninterested(unsigned int neighbor_id);
		void setRemoteInterested(unsigned int neighbor_id);
		void setRemoteUninterested(unsigned int neighbor_id);

		/* toggle choked */
		void setChoked(unsigned int neighbor_id);
		void setRemoteChoked(unsigned int neighbor_id);
		void setUnchoked(unsigned int neighbor_id);
		void setRemoteUnchoked(unsigned int neighbor_id);

		/* update neighbor state/bits */
		void recordBytesReceived(unsigned int neighbor_id, unsigned int bytes);
		void updateNeighborPiece(unsigned int neighbor_id, unsigned int piece_id);
		PeerState getNeighborState(unsigned int neighbor_id);

		/* IO operations */
		bool storePiece(unsigned int piece_id, const std::vector<char>& data);
		std::vector<char> readPiece(unsigned int piece_id);

		/* choking algorithm functions */
		std::vector<std::pair<int, double>> getInterestedNeighbors();
		void applyChoking(const std::vector<int>& preferred);
		void resetDownloadRates();
		int selectOptimisticNeighbor();
		void setOptimisticNeighbor(int chosen_neighbor_id);
		int selectRandom(const std::vector<uint8_t> bitfield);

		void broadcastHave(int piece_id);
		void broadcastNotInterested();
		void cancelPendingRequests();

	private:
		void initBitfield();
		void openFile();

		unsigned int peerID;
		const ConfigUtils& configUtils;

		std::mutex peer_mutex;
		std::vector<uint8_t> peer_bitfield;
		std::map<unsigned int, PeerState> neighbors;
		std::set<int> requestedPieces; 

		int opt_neighbor_id = -1;
		std::fstream fileObj;
};