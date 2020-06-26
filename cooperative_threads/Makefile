all: main.cpp scheduler/*
	make clean
	g++ --std=c++17 -pthread main.cpp scheduler/* -o main

clean:
	rm -f main
	rm -f scheduler/*.gch
