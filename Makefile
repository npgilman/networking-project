peerProcess: main.o
	g++ -o peerProcess main.o Logger.o

main.o: main.cpp Logger.h
	g++ -std=c++17 -c main.cpp

Logger.o: Logger.cpp Logger.h
	g++ -std=c++17 -c Logger.cpp