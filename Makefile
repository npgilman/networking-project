peerProcess: main.o LogUtils.o ConfigUtils.o ConnectionManager.o SessionManager.o
	g++ -o peerProcess main.o LogUtils.o ConfigUtils.o ConnectionManager.o SessionManager.o

main.o: main.cpp LogUtils.h ConfigUtils.h ConnectionManager.h SessionManager.h
	g++ -std=c++17 -c main.cpp

LogUtils.o: LogUtils.cpp LogUtils.h
	g++ -std=c++17 -c LogUtils.cpp

ConfigUtils.o: ConfigUtils.cpp ConfigUtils.h
	g++ -std=c++17 -c ConfigUtils.cpp

ConnectionManager.o: ConnectionManager.cpp ConnectionManager.h
	g++ -std=c++17 -c ConnectionManager.cpp

SessionManager.o: SessionManager.cpp SessionManager.h
	g++ -std=c++17 -c SessionManager.cpp
