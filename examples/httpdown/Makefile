
CPPFLAGS=-Wall -I../../.. -ggdb -std=c++0x -Wno-deprecated
LDFLAGS=-lcoroutine -lboost_context -levent -lrt -L../../ -lappframe -lgfe -lgcexsocket

CC=gcc
CXX=g++

%.o: %.c
	$(CC) -c -o $@ $(CPPFLAGS) $<

%.o: %.cpp
	$(CXX) -c -o $@ $(CPPFLAGS) $<

all: httpdown

# ------ lib ------

httpdown: httpdown.o http_server.o gfs_reader.o
	$(CXX) -o $@ $^ $(LDFLAGS)

clean:
	rm -f httpdown *.o



