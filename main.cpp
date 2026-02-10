#include <fstream>
#include <iostream>
#include <stdio.h>
#include <string>

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

/**  funciton prototypes */
void initialize();
void readCommonConfig();
void readPeerInfoConfig();

/**  main function */
int main() {
    initialize();


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
        exit(-1);
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
        exit(-1);
    }
    
    string temp;
    while (getline(config, temp)) {
        PeerInfo* peer = new PeerInfo;
        
        size_t idx = temp.find(' ', 0);
        peer->id = stoi(temp.substr(0, idx));
        // cout << "[" << 0 << "," << 0 + idx << ") " << temp.substr(0,idx) << endl;

        size_t previdx = idx;
        idx = temp.find(' ', previdx + 1);
        peer->hostname = temp.substr(previdx + 1, idx - (previdx + 1));
        // cout << "[" << previdx + 1 << "," << (previdx + 1) + (idx - (previdx + 1)) << ") " << temp.substr(previdx + 1, idx - previdx) << endl;


        previdx = idx;
        idx = temp.find(' ', previdx + 1);
        peer->port = stoi(temp.substr(previdx + 1, idx - (previdx + 1)));
        // cout << "[" << previdx + 1 << "," << (previdx + 1) + (idx - (previdx + 1)) << ") " << temp.substr(previdx + 1, idx - previdx) << endl;

        previdx = idx;
        peer->hasCompleteFile = stoi(temp.substr(previdx + 1, 1));
        // cout << "[" << previdx + 1 << "," << (previdx + 1) + (1) << ") " << temp.substr(previdx + 1, 1) << endl;

        // cout << "peer: id:" << peer->id << " hostname:" << peer->hostname << " port:" << peer->port << " hasCompleteFile:" << peer->hasCompleteFile << endl << endl;
    }

    config.close();
    cout << "Finished reading peerInfo.cfg" << endl;
}
