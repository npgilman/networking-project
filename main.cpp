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

// imports from defined classes
#include "LogUtils.h"
#include "ConfigUtils.h"
#include "ConnectionManager.h"

#define MAX_CONNECTIONS 10

/* Utilities */
LogUtils* logUtils = nullptr;
ConfigUtils* configUtils = nullptr;

/* Helper Functions */
int connectTo(int peerProcessID, PeerInfo* p_info);
void handleIncomingConnection(int new_fd, int peerProcessID);

/**  main function */
int main(int argc, char** argv) {
    if (argc == 1) {
        std::cerr << "Needs a processID number to start" << std::endl;
        exit(2);
    }

    int peerID = std::atoi(argv[1]);
    std::string peerIDString = argv[1];

    logUtils = new LogUtils(peerProcessID, "log_peer_" + processIDString + ".log");
    configUtils = new ConfigUtils();

    // client.c
    // needs to connect to all previously initialized peer process
    //      [0, .., n-1]
    const unsigned int index = configUtils->getPeerIndex(peerProcessID);
    PeerInfo* p_info = configUtils->getPeer(index);
    for (int i = 0; i < index; i++) {
        if (!fork()) {
            // initConnection 
            connectTo(peerProcessID, configUtils->getPeer(i));
        }
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
    socklen_t sin_size;

    while (true) {
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("accept");
            continue;
        }

        struct sockaddr *temp = (struct sockaddr *)&their_addr;
        void *var = (temp->sa_family == AF_INET)
                        ? (void*) &(((struct sockaddr_in*)temp)->sin_addr)
                        : (void*) &(((struct sockaddr_in6*)temp)->sin6_addr);
        
        inet_ntop(their_addr.ss_family, var, ipstr, sizeof ipstr);
        printf("server: got connection from %s\n", ipstr);

        printf("server: connected from '%d'\n", pID);
        logUtils->logConnectRecv((unsigned int) pID);

        if (!fork()) {
            close(sockfd);
            handleIncomingConnection(new_fd, peerProcessID);
            exit(0);
        }
        close(new_fd); 
    }

    logUtils->closeLog();
    
    return 0;
}

void handleIncomingConnection(int new_fd, int peerProcessID) {
    ConnectionManager conn(new_fd);

    unsigned int remotePeerID;

    if (!conn.receiveHandshakeMessage(remotePeerID)) {
        close(new_fd);
        return;
    }

    if (!conn.sendHandshakeMessage(peerID)) {
        close(new_fd);
        return;
    }

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

    std::cout << "Connected to " << s << "\n";

    ConnectionManager conn(sockfd);

    if (!conn.sendHandshakeMessage(peerProcessID)) {
        close(sockfd);
        return 3;
    }

    unsigned int remotePeerID;
    if (!conn.receiveHandshakeMessage(remotePeerID)) {
        close(sockfd);
        return 4;
    }

    std::cout << "Handshake complete with peer " << remotePeerID << "\n";

    conn.sendMessage(MessageType::INTERESTED, {});

    MessageType type;
    std::vector<char> payload;
    while (conn.receiveMessage(type, payload)) {
        std::cout << "Client received message type "
                  << static_cast<int>(type)
                  << " payload size " << payload.size() << "\n";
    }

    close(sockfd);
    return 0;
}
