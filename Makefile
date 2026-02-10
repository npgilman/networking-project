project1: main.o
	g++ -o project1 main.o 

main.o: main.cpp
	g++ -std=c++17 -c main.cpp
