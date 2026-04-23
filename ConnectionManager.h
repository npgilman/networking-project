#pragma once

#include <arpa/inet.h>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>
#include <vector>

enum class MessageType : uint8_t {
	CHOKE           = 0,
	UNCHOKE         = 1,
	INTERESTED      = 2, 
	NOT_INTERESTED  = 3,
	HAVE            = 4,
	BITFIELD        = 5,
	REQUEST         = 6,
	PIECE           = 7
};

class ConnectionManager {
	public:
		ConnectionManager(int socket_fd) {
			this->socket_fd = socket_fd;
		}

		bool sendHandshakeMessage(int peerID);
		bool receiveHandshakeMessage(int& peerID);
		bool sendMessage(MessageType type, std::vector<char>& payload);
		bool receiveMessage(MessageType& type, std::vector<char>& payload);

	private:
		struct msg_Handshake {
			char header[18];
			char zeroBits[10];
			unsigned int peerId;


			msg_Handshake(int peerID) {
				std::memcpy(header, HANDSHAKE_HEADER, 18);
				std::memset(zeroBits, 0, 10);
				peerId = htonl(peerID);
			}
		};

		static constexpr char HANDSHAKE_HEADER[18] = "P2PFILESHARINGPROJ"; // 18 bytes
		static constexpr unsigned int MAX_CONNECTIONS = 10;
		static constexpr unsigned int MAX_DATA_SIZE = 100;

		int socket_fd;
		bool sendAll(void* buffer, int length);
		bool receiveAll(void* buffer, int length);
};

