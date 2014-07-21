
CPPFLAGS=-Wall -I. -ggdb
LDFLAGS=-lboost_context

CC=gcc
CXX=g++

%.o: %.c
	$(CC) -c -o $@ $(CPPFLAGS) $<

%.o: %.cpp
	$(CXX) -c -o $@ $(CPPFLAGS) $<

all: main

main:main.o core.o
	$(CXX) -o $@ $^ $(LDFLAGS)

clean:
	rm -f main *.o



