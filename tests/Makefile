CC=g++
FLAGS=-std=c++14

debug: FLAGS+=-g
debug: tests

tests: tests.cpp ../cppmidi.h
	$(CC) $(FLAGS) -o tests tests.cpp 
clean:
	rm tests 
