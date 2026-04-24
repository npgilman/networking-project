#include "SessionManager.h"


unsigned int SessionManager::actualPieceSize(unsigned int index) {
	if (index < configUtils.getNumPieces() - 1) {
		return configUtils.getPieceSize();
	}
	return configUtils.getFileSize() - ((configUtils.getNumPieces() - 1) * configUtils.getPieceSize());
}

void SessionManager::initBitfield() {
	unsigned int numPieces = configUtils.getNumPieces();
	unsigned int numBytes = (numPieces + 7) / 8;

	bool hasFile = configUtils.hasCompleteFile(peerID);
	uint8_t value = (hasFile) ? 0xFF : 0x00;
	for (int i = 0; i < numBytes; i++){
		peer_bitfield.push_back(value);
		// when on last bitfield, zero bits that do not map to file
		if (hasFile  && (numPieces % 8 != 0) && (i == numBytes - 1)) {
			unsigned int bitsToReset = 8 - (numPieces % 8);
			peer_bitfield[i] = static_cast<uint8_t>(0xFF << bitsToReset);
		}
	}
}

void SessionManager::openFile() {
	std::string filePath = std::to_string(peerID) + "/" + "thefile";

	bool hasFile = configUtils.hasCompleteFile(peerID);
	if (hasFile) {
		fileObj.open(filePath, std::ios::in | std::ios::out | std::ios::binary);
		// if cannot open, error
		if (!fileObj.is_open()) {
			exit(1);
		}
	}  else {
		fileObj.open(filePath, std::ios::in | std::ios::out | std::ios::binary | std::ios::trunc);

		// file file to specified size and use later
        unsigned int chunkSize = 4096;
        std::vector<char> zeros(chunkSize, 0);
        unsigned int remaining = configUtils.getFileSize();
        while (remaining > 0) {
            unsigned int toWrite = std::min(remaining, chunkSize);
            fileObj.write(zeros.data(), toWrite);
            remaining -= toWrite;
        }
        fileObj.flush();
	}
}

bool SessionManager::hasPiece(uint32_t pieceIndex) {
	std::lock_guard<std::mutex> lock(peer_mutex);
	uint32_t byteIndex = pieceIndex / 8;
	uint32_t bitPos = 7 - (pieceIndex % 8);
	return (peer_bitfield[byteIndex] >> bitPos) & 1;
}

bool SessionManager::hasCompleteFile() {
	if (configUtils.hasCompleteFile(peerID)) {
		return true;
	}

	std::lock_guard<std::mutex> lock(peer_mutex);
	unsigned int numPieces = configUtils.getNumPieces();
	for (int i = 0; i < numPieces; i++) {
		int byteIndex = i / 8;
		int bitPos = 7 - (i % 8);
		if (((peer_bitfield[byteIndex] >> bitPos) & 1) != 1) {
			return false;
		}
	}

	// configUtils.setComplete(peerID);
	return true;

}

bool SessionManager::hasAnyPieces() {
	std::lock_guard<std::mutex> lock(peer_mutex);
	for (uint8_t byte : peer_bitfield) {
		if (byte > 0)
			return true;
	}
	return false;
}

bool SessionManager::isInteresting(std::vector<uint8_t>& neighbor_bitfield) {
	std::lock_guard<std::mutex> lock(peer_mutex);
	unsigned int numPieces = configUtils.getNumPieces();
	for (int i = 0; i < numPieces; i++) {
		int byteIndex = i / 8;
		int bitPos = 7 - (i % 8);

		bool weLack = (((peer_bitfield[byteIndex] >> bitPos) & 1) != 1);
		bool theyLack = (((neighbor_bitfield[byteIndex] >> bitPos) & 1) != 1);
		if (weLack && !theyLack) {
			return true;
		}
	}
	return false;
}

std::vector<uint8_t> SessionManager::myBitfield() {
	std::lock_guard<std::mutex> lock(peer_mutex);
	return peer_bitfield;
}

void SessionManager::addNeighbor(unsigned int neighbor_id, const std::vector<uint8_t>& neighbor_bitfield) {
	std::lock_guard<std::mutex> lock(peer_mutex);
	PeerState& peerState = neighbors[neighbor_id];
	peerState.bitfield = neighbor_bitfield;
}

void SessionManager::setInterested(unsigned int neighbor_id) {
	std::lock_guard<std::mutex> lock(peer_mutex);
	neighbors[neighbor_id].interested = true;
}
void SessionManager::setUninterested(unsigned int neighbor_id) {
	std::lock_guard<std::mutex> lock(peer_mutex);
	neighbors[neighbor_id].interested = false;
}
void SessionManager::setRemoteInterested(unsigned int neighbor_id) {
	std::lock_guard<std::mutex> lock(peer_mutex);
	neighbors[neighbor_id].remote_interested = true;
}
void SessionManager::setRemoteUninterested(unsigned int neighbor_id) {
	std::lock_guard<std::mutex> lock(peer_mutex);
	neighbors[neighbor_id].remote_interested = false;
}
void SessionManager::setChoked(unsigned int neighbor_id) {
	std::lock_guard<std::mutex> lock(peer_mutex);
	neighbors[neighbor_id].choked = true;
}
void SessionManager::setUnchoked(unsigned int neighbor_id) {
	std::lock_guard<std::mutex> lock(peer_mutex);
	neighbors[neighbor_id].choked = false;
}
void SessionManager::setRemoteChoked(unsigned int neighbor_id) {
	std::lock_guard<std::mutex> lock(peer_mutex);
	neighbors[neighbor_id].remote_choked = true;
}
void SessionManager::setRemoteUnchoked(unsigned int neighbor_id) {
	std::lock_guard<std::mutex> lock(peer_mutex);
	neighbors[neighbor_id].remote_choked = false;
}


void SessionManager::recordBytesReceived(unsigned int peerID, unsigned int bytes) {
    // downloadRate is used by the unchoke timer to pick preferred neighbors.
    // For now we accumulate raw bytes; the timer will divide by the interval.
    std::lock_guard<std::mutex> lock(peer_mutex);
    neighbors[peerID].downloadRate += bytes;
}

void SessionManager::updateNeighborPiece(unsigned int neighbor_id, unsigned int piece_id) {
    std::lock_guard<std::mutex> lock(peer_mutex);
    auto it = neighbors.find(neighbor_id);
    if (it == neighbors.end()) return;

    std::vector<uint8_t>& bitfield = it->second.bitfield;
    bitfield[piece_id / 8] = bitfield[piece_id / 8] | (1 << (7 - (piece_id % 8)));
}

PeerState SessionManager::getNeighborState(unsigned int neighbor_id) {
    std::lock_guard<std::mutex> lock(peer_mutex);
    auto it = neighbors.find(neighbor_id);
    return it->second;
}

bool SessionManager::storePiece(unsigned int piece_id, const std::vector<char>& data) {
	std::lock_guard<std::mutex> lock(peer_mutex);

	unsigned int offset = piece_id * configUtils.getPieceSize();
	fileObj.seekp(offset);

	fileObj.write(data.data(), data.size());
	fileObj.flush();

    peer_bitfield[piece_id / 8] = peer_bitfield[piece_id / 8] | (1 << (7 - (piece_id % 8)));
    requestedPieces.erase(piece_id);
    return true;

}

std::vector<char> SessionManager::readPiece(unsigned int piece_id) {
	std::lock_guard<std::mutex> lock(peer_mutex);
	unsigned int offset = piece_id * configUtils.getPieceSize();
	unsigned int size = actualPieceSize(piece_id);

	fileObj.seekg(offset);

	std::vector<char> buf(size);
	fileObj.read(buf.data(), size);
	return buf;
}

bool SessionManager::allPeersComplete() {
	std::lock_guard<std::mutex> lock(peer_mutex);
	unsigned int numPieces = configUtils.getNumPieces();
	unsigned int numBytes = (numPieces + 7) / 8;

	std::vector<uint8_t> completeMask(numBytes, 0xFF);
	if (numPieces % 8 != 0) {
		unsigned int bitsToReset = 8 - (numPieces % 8);
		completeMask.back() = static_cast<uint8_t>(0xFF << bitsToReset);
	}

	if (peer_bitfield != completeMask) {
		return false;
	}

	for (auto& pair : neighbors) {
	    auto& peer_stat = pair.second;
	    if (peer_stat.bitfield != completeMask) {
	        return false;
	    }
	}

	return true;
}

void SessionManager::applyChoking(const std::vector<int>& preferred) {
	std::lock_guard<std::mutex> lock(peer_mutex);

	std::set<int> preferred_neighbors_set(preferred.begin(), preferred.end());
	for (auto& pair : neighbors) {
		bool bChoke = (preferred_neighbors_set.find(pair.first) == preferred_neighbors_set.end()) && (opt_neighbor_id != pair.first);

		MessageType message_type;
		if (bChoke && !pair.second.remote_choked) {
			pair.second.remote_choked = true;
			pair.second.conn->sendMessage(MessageType::CHOKE, {});
		} else if (!bChoke && pair.second.remote_choked) {
			pair.second.remote_choked = false;
			pair.second.conn->sendMessage(MessageType::UNCHOKE, {});
		}
	}
}

void SessionManager::setNeighborConn(unsigned int neighbor_id, ConnectionManager* conn) {
	std::lock_guard<std::mutex> lock(peer_mutex);
	neighbors[neighbor_id].conn = conn;	
}

unsigned int SessionManager::countPieces() {
	std::lock_guard<std::mutex> lock(peer_mutex);
	unsigned int numPieces = configUtils.getNumPieces();
	unsigned int count = 0;

	for (int i = 0; i < numPieces; i++) {
		int byte = i/8;
		int bit = 7 - (i % 8);
		if ((peer_bitfield[byte] >> bit) & 1) {
			count++;
		}
	}
	return count;
}

void SessionManager::broadcastHave(int piece_id) {
	std::lock_guard<std::mutex> lock(peer_mutex);

	uint32_t network_piece_id = htonl(piece_id);
	std::vector<char> payload(4);
	std::memcpy(payload.data(), &network_piece_id, 4);

	for (auto& pair : neighbors) {
		pair.second.conn->sendMessage(MessageType::HAVE, payload);
	}
}

std::vector<std::pair<int, double>> SessionManager::getInterestedNeighbors() {
	std::lock_guard<std::mutex> lock(peer_mutex);
	std::vector<std::pair<int,double>> result;
	for (auto& pair : neighbors) {
		if (pair.second.remote_interested) {
			result.push_back({pair.first, pair.second.downloadRate});
		}
	}

	return result;
}

void SessionManager::resetDownloadRates() {
	std::lock_guard<std::mutex> lock(peer_mutex);
	for (auto& pair : neighbors) {
		pair.second.downloadRate = 0.0;
	}
}

int SessionManager::selectRandom(const std::vector<uint8_t> bitfield) {
	std::lock_guard<std::mutex> lock(peer_mutex);
	unsigned int numPieces = configUtils.getNumPieces();

	std::vector<int> candidates;
	for (int i = 0; i < numPieces; i++) {
		int byteIndex = i / 8;
		int bitPos = 7 - (i % 8);

		bool weLack = (((peer_bitfield[byteIndex] >> bitPos) & 1) != 1);
		bool theyLack = (((bitfield[byteIndex] >> bitPos) & 1) != 1);
		bool requested = requestedPieces.count(i) > 0;
		if (weLack && !theyLack && !requested) {
			candidates.push_back(i);
		}
	}

	if (candidates.empty()) return -1;
	std::random_device rd;
	std::shuffle(candidates.begin(), candidates.end(), std::mt19937{rd()});
	requestedPieces.insert(candidates[0]);
	return candidates[0];
}

int SessionManager::selectOptimisticNeighbor() {
	std::lock_guard<std::mutex> lock(peer_mutex);

	std::vector<int> candidates;
	for (auto& pair : neighbors) {
		if (pair.second.remote_choked && pair.second.remote_interested) {
			candidates.push_back(pair.first);
		}

	}

	if (candidates.empty()) return -1;
	std::random_device rd;
	std::shuffle(candidates.begin(), candidates.end(), std::mt19937{rd()});
	return candidates[0];
}

void SessionManager::setOptimisticNeighbor(int chosen_neighbor_id) {
	std::lock_guard<std::mutex> lock(peer_mutex);
	opt_neighbor_id = chosen_neighbor_id;
}

void SessionManager::cancelPendingRequests() {
	std::lock_guard<std::mutex> lock(peer_mutex);
	requestedPieces.clear();
}