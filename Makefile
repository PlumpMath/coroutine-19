
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

.PHONY: test

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



