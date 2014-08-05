
CPPFLAGS=-Wall -I../.. -ggdb -std=c++0x
LIBRT=$(shell uname|gawk '{if($$1=="Linux"){print "-lrt";exit;}}')
LDFLAGS=-lboost_context -levent $(LIBRT) -L..

CC=gcc
CXX=g++

%.o: %.c
	$(CC) -c -o $@ $(CPPFLAGS) $<

%.o: %.cpp
	$(CXX) -c -o $@ $(CPPFLAGS) $<

all: fsm

fsm: fsm.o
	$(CXX) -o $@ $^ -lcoroutine $(LDFLAGS)

clean:
	rm -f fsm *.o



