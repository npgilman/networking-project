#include "SessionManager.h"


unsigned int SessionManager::actualPieceSize(unsigned int index) {
	if (index < configUtils.getNumPieces()) {
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
		fileObj.open(filePath);
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
