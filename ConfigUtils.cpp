#include "ConfigUtils.h"

PeerInfo* ConfigUtils::getPeer(int i) {
	return peerInfo[i];
}

unsigned int ConfigUtils::getPeerIndex(unsigned int peerID) {
	return this->peerID2idx[peerID];
}

void ConfigUtils::readCommonConfig() {
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

void ConfigUtils::readPeerInfoConfig() {
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

