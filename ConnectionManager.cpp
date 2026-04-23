#include "ConnectionManager.h"

bool ConnectionManager::sendAll(void* buffer, int length) {
	int total = 0;
    char* buf = static_cast<char*>(buffer);

    while (total < length) {
        ssize_t n = send(socket_fd, buf + total, length - total, 0);
        if (n <= 0) {
            return false;
        }
        total += n;
    }
    return true;
}

bool ConnectionManager::receiveAll(void* buffer, int length) {
    int total = 0;
    char* buf = static_cast<char*>(buffer);

    while (total < length) {
        ssize_t n = recv(socket_fd, buf + total, length - total, 0);
        if (n <= 0) {
            return false;
        }
        total += n;
    }
    return true;
}

bool ConnectionManager::sendHandshakeMessage(int peerID) {
	msg_Handshake hs(peerID);

	if (!sendAll(&hs, sizeof(hs))) {
		perror("Sending handshake");
		return;
	}
}

bool ConnectionManager::receiveHandshakeMessage(int& peerID) {
	msg_Handshake hs(0);
	int numbytes;

	if (!recvAll(&hs, sizeof(hs))) {
        perror("Receiving handshake");
        return;
    } 

	if (std::memcmp(hs.header, HANDSHAKE_HEADER, 18) != 0) {
		perror("Invalid Handshake Header")
		return;
    }

    for (int i = 0; i < 10; i++) {
    	if (hs.zeroBits[i] != 0) {
    		// fail
    		perror("Zero bits")
    		return;
    	}
    }

    peerID = ntohl(hs.peerId);
}

bool ConnectionManager::sendMessage(MessageType type, std::vector<char>& payload) {
	uint32_t length = 1 + payload.size();
	uint32_t net_length = htonl(length);

	// build a buffer to send to peer
    std::vector<char> buf(4 + length);
    std::memcpy(buf.data(), &net_length, 4);
    buf[4] = static_cast<uint8_t>(type);
    if (!payload.empty()) {
        std::memcpy(buf.data() + 5, payload.data(), payload.size());
    }

    if (!sendAll(buf.data(), buf.size())) {
        perror("sendMessage");
        return false;
    }
	return true;
}

bool ConnectionManager::receiveMessage(MessageType& type, std::vector<char>& payload) {
    uint32_t net_length;

    if (!recvAll(&net_length, 4)) {
        perror("receiveMessage length");
        return false;
    }

    uint32_t length = ntohl(net_length);
    if (length < 1) {
    	perror("Invalid message length");
        return false;
    }

    std::vector<char> buf(length);
    if (!recvAll(buf.data(), length)) {
        perror("receiveMessage body");
        return false;
    }

    type = static_cast<MessageType>(buf[0]);
    payload.assign(buf.begin() + 1, buf.end());
    return true;
}

