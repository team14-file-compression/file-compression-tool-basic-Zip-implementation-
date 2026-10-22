# Makefile - build the tool and the tests
#   make        -> builds ./huffzip
#   make test   -> builds and runs the unit tests
#   make clean  -> removes build output
CXX      ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -O2
INCLUDES  = -Iinclude

LIB_SRC = $(filter-out src/main.cpp, $(wildcard src/*.cpp))

all: huffzip

huffzip: $(wildcard src/*.cpp) $(wildcard include/*)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(wildcard src/*.cpp) -o huffzip

run_tests: $(LIB_SRC) $(wildcard tests/*.cpp) $(wildcard include/*)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LIB_SRC) $(wildcard tests/*.cpp) -o run_tests

test: run_tests
	./run_tests

clean:
	rm -f huffzip huffzip.exe run_tests run_tests.exe

.PHONY: all test clean
