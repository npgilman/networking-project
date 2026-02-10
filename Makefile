peerProcess: main.o
	g++ -o peerProcess main.o 

main.o: main.cpp
	g++ -std=c++17 -c main.cpp
