
CPPFLAGS=-Wall -I../../.. -ggdb
LDFLAGS=-lboost_context -levent -L../../ -lcoroutine

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



