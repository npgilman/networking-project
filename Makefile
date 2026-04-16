peerProcess: main.o LogUtils.o ConfigUtils.o
	g++ -o peerProcess main.o LogUtils.o ConfigUtils.o

main.o: main.cpp LogUtils.h ConfigUtils.h
	g++ -std=c++17 -c main.cpp

LogUtils.o: LogUtils.cpp LogUtils.h
	g++ -std=c++17 -c LogUtils.cpp

ConfigUtils.o: ConfigUtils.cpp ConfigUtils.h
	g++ -std=c++17 -c ConfigUtils.cpp
