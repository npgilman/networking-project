#include <ctime>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <map>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <unistd.h>

// imports from beej's guide to networkign programming
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>

// imports from defined classes
#include "Logger.h"

#define MAX_CONNECTIONS 10
#define MAX_DATA_SIZE 100

// using namespace std;

/** configuration variables */
/* Common.cfg vars */
unsigned int numPreferredNeighbors = 0;
unsigned int unchokingInterval = 999999;
unsigned int optimisticUnchokingInterval = 999999; 
std::string fileName = "";
unsigned int fileSize;
unsigned int pieceSize;

/* PeerInfo.cfg vars */
struct PeerInfo {
    unsigned int id;
    std::string hostname;
    std::string port;
    bool hasCompleteFile;
} typedef PeerInfo;
std::vector<PeerInfo*> peerInfo;
std::map<unsigned int, unsigned int> peerID2idx;

/* Logging utilities */
Logger* loggingUtil = nullptr;

/**  funciton prototypes */
void initialize();
void readCommonConfig();
void readPeerInfoConfig();


/**  main function */
int main(int argc, char** argv) {
    initialize();
    
    if (argc == 1) {
        std::cerr << "Needs a processID number to start" << std::endl;
        exit(2);
    }

    std::string processIDString = std::string(argv[1]);
    int peerProcessID = atoi(argv[1]);
    std::cout << peerProcessID << std::endl;
    
    loggingUtil = new Logger(peerProcessID, "log_peer_" + processIDString + ".log");

    // client.c
    for (int i = 0; i < peerID2idx[peerProcessID]; i++) {
        if (!fork()) {
            int sockfd, numbytes;
            char buf[MAX_DATA_SIZE];
            struct addrinfo hints, *servinfo, *p;
            int rv;
            char s[INET6_ADDRSTRLEN];

            memset(&hints, 0, sizeof hints);
            hints.ai_family = AF_UNSPEC;
            hints.ai_socktype = SOCK_STREAM;

            PeerInfo* p_info = peerInfo[i];
            if ((rv = getaddrinfo(p_info->hostname.c_str(), p_info->port.c_str(), &hints, &servinfo)) != 0) {
                fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
                return 1;
            }

            for (p = servinfo; p != NULL; p = p->ai_next) {
                if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
                    perror("client: socket");
                    continue;
                }

                struct sockaddr *temp = (struct sockaddr *)p->ai_addr;
                // TODO - revisit this
                void *var = (temp->sa_family == AF_INET) ? (void*) &(((struct sockaddr_in*)temp)->sin_addr) : 
                                                                (void*) &(((struct sockaddr_in6*)temp)->sin6_addr);

                inet_ntop(p->ai_family, var, s, sizeof s);
                printf("client: attempting connection to %s\n", s);

                if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
                    perror("client: connect");
                    close(sockfd);
                    continue;
                }

                break;
            }

            if (p == NULL) {
                fprintf(stderr, "client: failed to connect\n");
                return 2;
            }

            struct sockaddr *temp = (struct sockaddr *)p->ai_addr;
            // TODO - revisit this
            void *var = (temp->sa_family == AF_INET) ? (void*) &(((struct sockaddr_in*)temp)->sin_addr) : 
                                                            (void*) &(((struct sockaddr_in6*)temp)->sin6_addr);


            inet_ntop(p->ai_family, var, s, sizeof s);
            freeaddrinfo(servinfo);   

            printf("client: connected to %s\n", s);
            loggingUtil->logConnectMake(peerProcessID, p_info->id);

            int pID = htonl(peerProcessID);
            if (send(sockfd, &pID, sizeof(pID), 0) == -1) {
                perror("send");
            }

            while (true) {
                numbytes = recv(sockfd, buf, MAX_DATA_SIZE - 1, 0);
                if (numbytes == -1) { perror("recv"); break; }
                if (numbytes == 0) { std::cout << "Server closed connection\n"; break; }
                buf[numbytes] = '\0';

                recv(sockfd, &pID, sizeof(pID), 0); 
                pID = ntohl(pID);

                printf("client: received '%s' from peerID: %d\n", buf, pID);
            }

            close(sockfd);
            return 0;
        }
    }

    // addr info variables
    int status;
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    std::cout << "Desired port number for server: ";
    std::cout << peerInfo[peerID2idx[peerProcessID]]->port.c_str();
    std::cout << std::endl;

    if ((status = getaddrinfo(NULL, peerInfo[peerID2idx[peerProcessID]]->port.c_str(), &hints, &res)) != 0) {
        fprintf(stderr, "gai error: %s\n", gai_strerror(status));
        exit(100);
    }
    
    // socket variables
    int sockfd, new_fd;
    socklen_t sin_size;
    struct sigaction sa;
    int yes = 1;
    int rv;

    // bind to first possible network address
    struct addrinfo *p;
    char ipstr[INET6_ADDRSTRLEN];
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

        // inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
        // printf("\t%s: %s\n", ipver, ipstr);
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
    while (1) {
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("accept");
            continue;
        }

        struct sockaddr *temp = (struct sockaddr *)&their_addr;
        // TODO - revisit this
        void *var = (temp->sa_family == AF_INET) ? (void*) &(((struct sockaddr_in*)temp)->sin_addr) : 
                                                        (void*) &(((struct sockaddr_in6*)temp)->sin6_addr);
        
        inet_ntop(their_addr.ss_family, var, ipstr, sizeof ipstr);
        printf("server: got connection from %s\n", ipstr);

        // TODO - Need to receive a message with pID of connector
        int pID;
        recv(new_fd, &pID, sizeof pID, 0);
        pID = ntohl(pID);

        printf("server: connected from '%d'\n", pID);
        loggingUtil->logConnectRecv(peerProcessID, (unsigned int) pID);

        if (!fork()) {
            close(sockfd);

            while (true) {
                if (send(new_fd, "Hello, World!", 13, 0) == -1) {
                    perror("send");
                }
                pID = htonl(peerProcessID);
                if (send(new_fd, &pID, sizeof(pID), 0) == -1) {
                    perror("send");
                }
                sleep(5);
            }
            int numbytes;
            char buf[MAX_DATA_SIZE];
            if ((numbytes = recv(new_fd, buf, MAX_DATA_SIZE - 1, 0)) == -1) {
                perror("recv");
                exit(1);
            }

            buf[numbytes] = '\0';
            printf("server: received '%s'\n", buf);

            close(new_fd);
            exit(0);
        }
        close(new_fd); 
    }

    loggingUtil->closeLog();
    
    return 0;
}

/**  function definitions */
void initialize() {
    readCommonConfig();
    readPeerInfoConfig();
}


void readCommonConfig() {
    std::string fileName = "Common.cfg";
    std::fstream config(fileName);

    if (!config) {
        std::cerr << "Failed to open Common.cfg" << std::endl;
        exit(1);
    }

    std::string temp;
    std::string value;
    
    getline(config, temp);
    value = temp.substr(temp.find(' ', 0) + 1, temp.length() - 1);
    numPreferredNeighbors = stoi(value);
   
    getline(config, temp);
    value = temp.substr(temp.find(' ', 0) + 1, temp.length() - 1);
    unchokingInterval = stoi(value);
   
    getline(config, temp);
    value = temp.substr(temp.find(' ', 0) + 1, temp.length() - 1);
    optimisticUnchokingInterval = stoi(value);

    getline(config, temp);
    value = temp.substr(temp.find(' ', 0) + 1, temp.length() - 1);
    fileName = value;

    getline(config, temp);
    value = temp.substr(temp.find(' ', 0) + 1, temp.length() - 1);
    fileSize = stoi(value);
   
    getline(config, temp);
    value = temp.substr(temp.find(' ', 0) + 1, temp.length() - 1);
    pieceSize = stoi(value);

    config.close();
    std::cout << "Finished reading Common.cfg" << std::endl;
}

void readPeerInfoConfig() {
    std::string fileName = "PeerInfo.cfg";
    std::fstream config(fileName);

    if (!config) {
        std::cerr << "Failed to open Common.cfg" << std::endl;
        exit(1);
    }
    
    std::string temp;
    unsigned int vectIndex = 0;
    while (getline(config, temp)) {
        PeerInfo* peer = new PeerInfo;
        
        size_t idx = temp.find(' ', 0);
        peer->id = stoi(temp.substr(0, idx));

        size_t previdx = idx;
        idx = temp.find(' ', previdx + 1);
        peer->hostname = temp.substr(previdx + 1, idx - (previdx + 1));

        previdx = idx;
        idx = temp.find(' ', previdx + 1);
        peer->port = temp.substr(previdx + 1, idx - (previdx + 1));

        previdx = idx;
        peer->hasCompleteFile = stoi(temp.substr(previdx + 1, 1));

        peerInfo.push_back(peer);
        peerID2idx.emplace(peer->id, vectIndex++);
    }

    config.close();
    std::cout << "Finished reading peerInfo.cfg" << std::endl;
}

