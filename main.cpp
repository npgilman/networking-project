#include <random>
#include <iostream>
#include <iomanip>
#include <map>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <thread>
#include <mutex>

// imports from defined classes
#include "LogUtils.h"
#include "ConfigUtils.h"
#include "ConnectionManager.h"
#include "SessionManager.h"

#define MAX_CONNECTIONS 10

/* Utilities */
LogUtils* logUtils = nullptr;
ConfigUtils* configUtils = nullptr;
SessionManager* sessionManager = nullptr;

/* Thread Utilities */
std:: mutex thread_mutex;
std::vector<std::thread> threads;
std::atomic<bool> running{true};

/* Helper Functions */
int connectTo(int peerProcessID, PeerInfo* p_info);
void handleIncomingConnection(int new_fd, int peerProcessID);
void handleMessage(MessageType type, const std::vector<char>& payload, int remotePeerID, int peerProcessID, ConnectionManager& conn);
void runUnchokeAlgorithm(int peerProcessID);
void runOptimisticUnchokeAlgorithm(int peerProcessID);

/**  main function */
int main(int argc, char** argv) {
    if (argc == 1) {
        std::cerr << "Needs a processID number to start" << std::endl;
        exit(2);
    }

    int peerProcessID = std::atoi(argv[1]);
    std::string peerProcessIDString = argv[1];

    logUtils = new LogUtils(peerProcessID, "log_peer_" + peerProcessIDString + ".log");
    configUtils = new ConfigUtils();
    sessionManager = new SessionManager(peerProcessID, *configUtils);

    // client.c
    // needs to connect to all previously initialized peer process
    //      [0, .., n-1]
    const unsigned int index = configUtils->getPeerIndex(peerProcessID);
    PeerInfo* p_info = configUtils->getPeer(index);
    for (int i = 0; i < index; i++) {
        PeerInfo* neighbor = configUtils->getPeer(i);
        std::lock_guard<std::mutex> lock(thread_mutex);
        threads.emplace_back([peerProcessID, neighbor]() {
            connectTo(peerProcessID, neighbor);
        });
    }


    {
        std::lock_guard<std::mutex> lock(thread_mutex);
        threads.emplace_back([peerProcessID]() {
            while (running) {
                std::this_thread::sleep_for(
                    std::chrono::seconds(configUtils->getUnchokingInterval()));
                runUnchokeAlgorithm(peerProcessID);
            }
        });
    }

    {
        std::lock_guard<std::mutex> lock(thread_mutex);
        threads.emplace_back([peerProcessID]() {
            while (running) {
                std::this_thread::sleep_for(
                    std::chrono::seconds(configUtils->getOptimisticUnchokingInterval()));
                runOptimisticUnchokeAlgorithm(peerProcessID);
            }
        }); 
    }

    // addr info variables
    int status;
    struct addrinfo hints, *res, *p;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((status = getaddrinfo(NULL, p_info->port.c_str(), &hints, &res)) != 0) {
        fprintf(stderr, "gai error: %s\n", gai_strerror(status));
        exit(100);
    }
    
    // socket variables
    int sockfd;
    int yes = 1;

    // bind to first possible network address
    for (p = res; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("server:socket");
            continue;
        }
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break;

    }

    freeaddrinfo(res);

    if (p == NULL) {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }
    if (listen(sockfd, MAX_CONNECTIONS) == -1) {
        perror("listen");
        exit(1);
    }


    // TODO : Need code to clean up zombie threads
    struct sockaddr_storage their_addr;
    char ipstr[INET6_ADDRSTRLEN];
    while (running) {
        socklen_t sin_size = sizeof their_addr;
        int new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("accept");
            continue;
        }

        struct sockaddr *temp = (struct sockaddr *)&their_addr;
        void *var = (temp->sa_family == AF_INET)
                        ? (void*) &(((struct sockaddr_in*)temp)->sin_addr)
                        : (void*) &(((struct sockaddr_in6*)temp)->sin6_addr);
        
        inet_ntop(their_addr.ss_family, var, ipstr, sizeof ipstr);

        std::lock_guard<std::mutex> lock(thread_mutex);
        threads.emplace_back([new_fd, peerProcessID](){
            handleIncomingConnection(new_fd, peerProcessID);
        });
    }

    logUtils->closeLog();
    
    std::lock_guard<std::mutex> lock(thread_mutex);
    for (std::thread& t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }

    return 0;
}

void handleIncomingConnection(int new_fd, int peerProcessID) {
    ConnectionManager conn(new_fd);

    int remotePeerID;
    if (!conn.receiveHandshakeMessage(remotePeerID)) {
        close(new_fd);
        return;
    }

    if (!conn.sendHandshakeMessage(peerProcessID)) {
        close(new_fd);
        return;
    }

    logUtils->logConnectRecv((unsigned int) remotePeerID);
    std::cout << "Handshake complete with peer " << remotePeerID << "\n";

    if (sessionManager->hasAnyPieces()) {
        std::vector<uint8_t> bitfield = sessionManager->myBitfield();
        std::vector<char> payload(bitfield.begin(), bitfield.end());
        if (!conn.sendMessage(MessageType::BITFIELD, payload)) {
            close(new_fd);
            return;
        }
    }

    MessageType type;
    std::vector<char> payload;
    if (!conn.receiveMessage(type, payload)) {
        std::cout << "error from " << remotePeerID << std::endl;
        close(new_fd);
        return;
    }

    if (type == MessageType::BITFIELD) {
        std::cout << "bitfield from " << remotePeerID << std::endl;
        std::vector<uint8_t> incoming_bitfield(payload.begin(), payload.end());
        sessionManager->addNeighbor(remotePeerID, incoming_bitfield);

        if (sessionManager->isInteresting(incoming_bitfield)) {
            std::cout << " is interested in " << remotePeerID << std::endl;
            sessionManager->setInterested(remotePeerID);
            conn.sendMessage(MessageType::INTERESTED, {});
        } else {
            sessionManager->setUninterested(remotePeerID);
            conn.sendMessage(MessageType::NOT_INTERESTED, {});
        }
    } else {
        std::cout << " no bitfield from " << remotePeerID << std::endl;
        sessionManager->addNeighbor(remotePeerID, std::vector<uint8_t>((configUtils->getNumPieces()+7)/8, 0x00));
        handleMessage(type, payload, remotePeerID, peerProcessID, conn);
    }
    sessionManager->setNeighborConn(remotePeerID, &conn);

    // Main message loop.
    while (conn.receiveMessage(type, payload)) {
        handleMessage(type, payload, remotePeerID, peerProcessID, conn);
    }

    close(new_fd);

}

int connectTo(int peerProcessID, PeerInfo* p_info) {
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(p_info->hostname.c_str(), p_info->port.c_str(), &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            perror("client: connect");
            close(sockfd);
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        freeaddrinfo(servinfo);
        return 2;
    }

    inet_ntop(
        p->ai_family,
        (p->ai_family == AF_INET)
            ? (void*)&(((struct sockaddr_in*)p->ai_addr)->sin_addr)
            : (void*)&(((struct sockaddr_in6*)p->ai_addr)->sin6_addr),
        s, sizeof s
    );

    freeaddrinfo(servinfo);

    ConnectionManager conn(sockfd);

    if (!conn.sendHandshakeMessage(peerProcessID)) {
        close(sockfd);
        return 3;
    }

    int remotePeerID;
    if (!conn.receiveHandshakeMessage(remotePeerID)) {
        close(sockfd);
        return 4;
    }

    logUtils->logConnectMake(remotePeerID);
    std::cout << "Handshake complete with peer " << remotePeerID << "\n";

    if (sessionManager->hasAnyPieces()) {
        std::vector<uint8_t> bitfield = sessionManager->myBitfield();
        std::vector<char> payload(bitfield.begin(), bitfield.end());
        if (!conn.sendMessage(MessageType::BITFIELD, payload)) {
            close(sockfd);
            return 5;
        }
    }


    MessageType type;
    std::vector<char> payload;
    if (!conn.receiveMessage(type, payload)) {
        close(sockfd);
        return 6;
    }

    if (type == MessageType::BITFIELD) {
        std::cout << "bitfield from " << remotePeerID << std::endl;
        std::vector<uint8_t> incoming_bitfield(payload.begin(), payload.end());
        sessionManager->addNeighbor(remotePeerID, incoming_bitfield);

        if (sessionManager->isInteresting(incoming_bitfield)) {
            std::cout << " is interested in " << remotePeerID << std::endl;
            sessionManager->setInterested(remotePeerID);
            conn.sendMessage(MessageType::INTERESTED, {});
        } else {
            sessionManager->setUninterested(remotePeerID);
            conn.sendMessage(MessageType::NOT_INTERESTED, {});
        }
    } else {
        std::cout << "no bitfield from " << remotePeerID << std::endl;
        sessionManager->addNeighbor(remotePeerID, std::vector<uint8_t>((configUtils->getNumPieces()+7)/8, 0x00));
        handleMessage(type, payload, remotePeerID, peerProcessID, conn);
    }
    sessionManager->setNeighborConn(remotePeerID, &conn);

    while (conn.receiveMessage(type, payload)) {
        handleMessage(type, payload, remotePeerID, peerProcessID, conn);
    }

    close(sockfd);
    return 0;
}

void handleMessage(MessageType type, const std::vector<char>& payload, int remotePeerID, int peerProcessID, ConnectionManager& conn) {
    switch (type) {
        case MessageType::BITFIELD:
            {

            }
            break;
        case MessageType::CHOKE:
            {
                sessionManager->setChoked(remotePeerID);
                logUtils->logChoking(remotePeerID);
            }
            break;
        case MessageType::UNCHOKE:
            {
                sessionManager->setUnchoked(remotePeerID);
                logUtils->logUnchoking(remotePeerID);

                PeerState peer_state = sessionManager->getNeighborState(remotePeerID);
                int next_piece_id = sessionManager->selectRandom(peer_state.bitfield);
                if (next_piece_id >= 0) {
                    uint32_t network_next_piece_id = htonl((uint32_t) next_piece_id);
                    std::vector<char> requestMessage_Payload(4);
                    std::memcpy(requestMessage_Payload.data(), &network_next_piece_id, 4);
                    conn.sendMessage(MessageType::REQUEST, requestMessage_Payload);
                }
            }
            break;
        case MessageType::INTERESTED:
            {
                std::cout << "received intereted from " << remotePeerID << std::endl;
                sessionManager->setRemoteInterested(remotePeerID);
                logUtils->logRecvInterested(remotePeerID);
            }
            break;
        case MessageType::NOT_INTERESTED:
            {
                sessionManager->setRemoteUninterested(remotePeerID);
                logUtils->logRecvNotInterested(remotePeerID);
            }
            break;
        case MessageType::HAVE:
            {
                uint32_t piece_id;
                std::memcpy(&piece_id, payload.data(), 4);
                piece_id = ntohl(piece_id);
                logUtils->logRecvHave(remotePeerID, (unsigned int)piece_id);

                sessionManager->updateNeighborPiece(remotePeerID, piece_id);

                std::vector<uint8_t> neighbor_bitfield = sessionManager->getNeighborState(remotePeerID).bitfield;
                bool isInterested = sessionManager->isInteresting(neighbor_bitfield);
                if (isInterested) {
                    sessionManager->setInterested(remotePeerID);
                    conn.sendMessage(MessageType::INTERESTED, {});
                } else {
                    sessionManager->setUninterested(remotePeerID);
                    conn.sendMessage(MessageType::NOT_INTERESTED, {});
                }
            }
            break;
        case MessageType::REQUEST:
            {
                uint32_t piece_id;
                std::memcpy(&piece_id, payload.data(), 4);
                piece_id = ntohl(piece_id);

                if (sessionManager->getNeighborState(remotePeerID).remote_choked)
                    break;

                std::vector<char> piece_data = sessionManager->readPiece(piece_id);
                if (piece_data.empty())
                    break;

                std::vector<char> pieceMessage_Payload(4 + piece_data.size());
                std::memcpy(pieceMessage_Payload.data(), payload.data(), 4);
                std::memcpy(pieceMessage_Payload.data() + 4, piece_data.data(), piece_data.size());
                conn.sendMessage(MessageType::PIECE, pieceMessage_Payload);
            }
            break;
        case MessageType::PIECE:
            {
                uint32_t piece_id;
                std::memcpy(&piece_id, payload.data(), 4);
                piece_id = ntohl(piece_id);

                std::vector<char> data(payload.begin() + 4, payload.end());
                sessionManager->recordBytesReceived(remotePeerID, (unsigned int)data.size());
                sessionManager->storePiece(piece_id, data);

                logUtils->logDownloaded(remotePeerID, piece_id, sessionManager->countPieces());

                std::vector<char> haveMessage_Payload(4);
                std::memcpy(haveMessage_Payload.data(), payload.data(), 4);
                sessionManager->broadcastHave(piece_id);

                if (sessionManager->hasCompleteFile())
                    logUtils->logCompletion();
                if (sessionManager->allPeersComplete()) {
                    running = false;
                }

                PeerState peer_state = sessionManager->getNeighborState(remotePeerID);
                if (!peer_state.choked) {
                    int next_piece_id = sessionManager->selectRandom(peer_state.bitfield);
                    if (next_piece_id >= 0) {
                        uint32_t network_next_piece_id = htonl((uint32_t) next_piece_id);
                        std::vector<char> requestMessage_Payload(4);
                        std::memcpy(requestMessage_Payload.data(), &network_next_piece_id, 4);
                        conn.sendMessage(MessageType::REQUEST, requestMessage_Payload);
                    }
                }

            }
            break;
    }
}

void runUnchokeAlgorithm(int peerProcessID) {
    unsigned int k = configUtils->getNumPreferredNeighbors();
    std::vector<std::pair<int, double>> interested = sessionManager->getInterestedNeighbors();

    std::random_device rd;
    std::shuffle(interested.begin(), interested.end(), std::mt19937{rd()});
    if (!sessionManager->hasCompleteFile()) {
        std::sort(interested.begin(), interested.end(), 
            [](const auto& a, const auto& b) {
                return a.second > b.second;
            });
    }

    std::vector<int> preferred;
    for (int i = 0; i < k && i < interested.size(); i++) {
        preferred.push_back(interested[i].first);
    }
    std::cout << "Unchoke timer fired. Interested neighbors: " << interested.size()
          << " Preferred: " << preferred.size() << "\n";
    sessionManager->applyChoking(preferred);
    logUtils->logUpdatePrefNeighbors(std::vector<unsigned int>(preferred.begin(), preferred.end()));
    sessionManager->resetDownloadRates();
}

void runOptimisticUnchokeAlgorithm(int peerProcessID) {
    int chosen_neighbor_id = sessionManager->selectOptimisticNeighbor();
    if (chosen_neighbor_id < 0) return;

    sessionManager->setOptimisticNeighbor(chosen_neighbor_id);
    logUtils->logUpdateOptUnchokedNeighbor(chosen_neighbor_id);
}
