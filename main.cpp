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
#include "LogUtils.h"
#include "ConfigUtils.h"

#define MAX_CONNECTIONS 10
#define MAX_DATA_SIZE 100

/* Utilities */
LogUtils* logUtils = nullptr;
ConfigUtils* configUtils = nullptr;

/* Helper Functions */
int connectTo(int peerProcessID, PeerInfo* p_info);


/**  main function */
int main(int argc, char** argv) {

    if (argc == 1) {
        std::cerr << "Needs a processID number to start" << std::endl;
        exit(2);
    }

    std::string processIDString = std::string(argv[1]);
    int peerProcessID = atoi(argv[1]);
    std::cout << peerProcessID << std::endl;
    
    logUtils = new LogUtils(peerProcessID, "log_peer_" + processIDString + ".log");
    configUtils = new ConfigUtils();

    // client.c
    // needs to connect to all previously initialized peer process
    //      [0, .., n-1]
    const unsigned int index = configUtils->getPeerIndex(peerProcessID);
    for (int i = 0; i < index; i++) {
        if (!fork()) {
            // initConnection 
            connectTo(peerProcessID, configUtils->getPeer(i));
        }
    }

    // addr info variables
    int status;
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    PeerInfo* p_info = configUtils->getPeer(index);
    std::cout << "Desired port number for server: ";
    std::cout << p_info->port.c_str();
    std::cout << std::endl;

    if ((status = getaddrinfo(NULL, p_info->port.c_str(), &hints, &res)) != 0) {
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
        logUtils->logConnectRecv((unsigned int) pID);

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

    logUtils->closeLog();
    
    return 0;
}

int connectTo(int peerProcessID, PeerInfo* p_info) {
    int sockfd, numbytes;
    char buf[MAX_DATA_SIZE];
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
    logUtils->logConnectMake(p_info->id);

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