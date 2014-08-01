
CPPFLAGS=-Wall -I../../.. -ggdb -std=c++0x -Wno-deprecated #-D_FS_USE_GFS_
LDFLAGS=-L../../ -lcoroutine -lboost_context -levent -lrt \
        -lgfsclient -lappframe -lgfe -lgcexsocket -lxthread -llog4cplus

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



