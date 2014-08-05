
# ===== header start =====

CPPFLAGS=-Wall -I../../include -ggdb -std=c++0x -Wno-deprecated #-D_FS_USE_GFS_
LIBRT=$(shell uname|gawk '{if($$1=="Linux"){print "-lrt";exit;}}')
LDFLAGS=-L../.. -lcoroutine -lboost_context -levent $(LIBRT) \
        -lgfsclient -lappframe -lgfe -lgcexsocket -lxthread -llog4cplus

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

all: httpdown

httpdown: $(OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS)

clean:
	rm -f httpdown *.o

