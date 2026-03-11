#include <fstream>
#include <iostream>
#include <map>
#include <stdio.h>
#include <stdlib.h>
#include <string>

// imports from beej's guide to networkign programming
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>

using namespace std;

/** configuration variables */
/* Common.cfg vars */
unsigned int numPreferredNeighbors = 0;
unsigned int unchokingInterval = 999999;
unsigned int optimisticUnchokingInterval = 999999; 
string fileName = "";
unsigned int fileSize;
unsigned int pieceSize;

/* PeerInfo.cfg vars */
struct PeerInfo {
    unsigned int id;
    string hostname;
    unsigned int port;
    bool hasCompleteFile;
} typedef PeerInfo;
vector<PeerInfo*> peerInfo;
map<unsigned int, unsigned int> peerID2idx;

/**  funciton prototypes */
void initialize();
void readCommonConfig();
void readPeerInfoConfig();

/**  main function */
int main(int argc, char** argv) {
    initialize();
    
    if (argc == 1) {
        cerr << "Needs a processID number to start" << endl;
        exit(2);
    }

    int peerProcessID = atoi(argv[1]);
    cout << peerProcessID << endl;

    
    if (false) {
        /* setup listening struct */ 
        int status;
        struct addrinfo hints;
        struct addrinfo *res; // points to results
    
        memset(&hints, 0, sizeof hints); // empties struct
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = AI_PASSIVE; // binds to the IP of the machine, to use other IP ADDRESS change flag
    
        if ((status = getaddrinfo(NULL, peerInfo[peerID2idx[peerProcessID]]->port, &hints, &res)) != 0) {
            fprintf(stderr, "gai error: %s\n", gai_strerror(status));
            exit(100);
        }
        
        int sockfd;
        sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        bind(sockfd, res->ai_addr, res->ai_addrlen);
        
        // do error checking

        listen(sockfd, 10); // tell how many pendings connections to allow
    }

    // for now, assume peerProcess will always be in the file
    for (int i = 0; i < peerID2idx[peerProcessID]; i++) {
        /* setup connecting struct */
        int status;
        struct addrinfo hints;
        struct addrinfo *res; // points to results

        memset(&hints, 0, sizeof hints); // empties struct
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;

        PeerInfo* peer = peerInfo[i];
        char buffer[6];
        snprintf(buffer, 6, "%d", peer->port);

        status = getaddrinfo(peer->hostname.c_str(), buffer,  &hints, &res);

        // you should do error-checking on getaddrinfo(), and walk
        // the "res" linked list looking for valid entries instead of just
        // assuming the first one is good (like many of these examples do).
        // See the section on client/server for real examples.

        // returns socket descriptor or -1 on error
        int sock;
        sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

        // connect(sockfd, res->ai_addr, res->ai_addrlen);
        // check return value, -1 if error

    }
    /*
    determine which peerProcess this is
    check and connect to all previously started peerProcesses (oldID < thisID)
    wait and accept future connections

    foreach connection:
        send messages back and forth
    */
    
    return 0;
}

/**  function definitions */
void initialize() {
    readCommonConfig();
    readPeerInfoConfig();
}


void readCommonConfig() {
    string fileName = "Common.cfg";
    fstream config(fileName);

    if (!config) {
        cerr << "Failed to open Common.cfg" << endl;
        exit(1);
    }

    string temp;
    string value;
    
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
    cout << "Finished reading Common.cfg" << endl;
}

void readPeerInfoConfig() {
    string fileName = "PeerInfo.cfg";
    fstream config(fileName);

    if (!config) {
        cerr << "Failed to open Common.cfg" << endl;
        exit(1);
    }
    
    string temp;
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
        peer->port = stoi(temp.substr(previdx + 1, idx - (previdx + 1)));

        previdx = idx;
        peer->hasCompleteFile = stoi(temp.substr(previdx + 1, 1));

        peerID2idx.emplace(peer->id, vectIndex++);
    }

    config.close();
    cout << "Finished reading peerInfo.cfg" << endl;
}
