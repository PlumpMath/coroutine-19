
# ===== header start =====

CPPFLAGS=-Wall -I./include -ggdb -std=c++0x
LIBRT=$(shell uname|gawk '{if($$1=="Linux"){print "-lrt";exit;}}')
LDFLAGS=-lboost_context -levent $(LIBRT) -L.

CC=gcc
CXX=g++
AR=ar

%.o: %.c
	$(CC) -c -o $@ $(CPPFLAGS) $<

%.o: %.cpp
	$(CXX) -c -o $@ $(CPPFLAGS) $<

SRCS=$(wildcard src/*.cpp)
OBJS=$(patsubst %.cpp,%.o,$(SRCS))

.PHONY: test install

# ===== header end =====

all: libcoroutine.a

libcoroutine.a: $(OBJS)
	$(AR) rvc $@ $^

test:
	make -C test

clean:
	rm -f $(OBJS) libcoroutine.a
	make -C test clean
	make -C examples clean

# To change PREFIX, use make -e PREFIX="/opt/build-env" install
PREFIX=/usr/local/
install:
	cp -f include/coroutine $(PREFIX)/include/
	cp -f libcoroutine.a $(PREFIX)/lib/


