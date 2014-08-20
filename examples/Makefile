
# ===== header start =====

CPPFLAGS=-Wall -I../include -ggdb -std=c++0x
LIBRT=$(shell uname|gawk '{if($$1=="Linux"){print "-lrt";exit;}}')
LDFLAGS=-L.. -lcoroutine -lboost_context -levent $(LIBRT)

CC=gcc
CXX=g++
AR=ar

%.o: %.c
	$(CC) -c -o $@ $(CPPFLAGS) $<

%.o: %.cpp
	$(CXX) -c -o $@ $(CPPFLAGS) $<

SRCS=$(wildcard *.cpp)
OBJS=$(patsubst %.cpp,%.o,$(SRCS))

.PHONY: test

# ===== header end =====

all: fsm echoserver echoclient

fsm: fsm.o
	$(CXX) -o $@ $^ $(LDFLAGS)

echoserver:echoserver.o
	$(CXX) -o $@ $^ $(LDFLAGS)

echoclient:echoclient.o
	$(CXX) -o $@ $^ $(LDFLAGS)

clean:
	rm -f fsm echoserver echoclient *.o



