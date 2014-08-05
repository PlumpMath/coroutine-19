
CPPFLAGS=-Wall -I. -ggdb -std=c++0x
LIBRT=$(shell uname|gawk '{if($$1=="Linux"){print "-lrt";exit;}}')
LDFLAGS=-lboost_context -levent $(LIBRT) -L.

CC=gcc
CXX=g++
AR=ar

%.o: %.c
	$(CC) -c -o $@ $(CPPFLAGS) $<

%.o: %.cpp
	$(CXX) -c -o $@ $(CPPFLAGS) $<

all: lib

# ------ lib ------

lib: libcoroutine.a

libcoroutine.a: core.o sceduler.o event.o
	ar rvc libcoroutine.a $^

# ------ unit test ------

test: lib unittest
	./unittest

unittest: core_test.o sceduler_test.o dispatcher_test.o event_test.o
	$(CXX) -o $@ $^ -lcoroutine $(LDFLAGS) -lgtest -lgtest_main

clean:
	rm -f unittest *.o *.a



