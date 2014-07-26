
CPPFLAGS=-Wall -I. -ggdb
LDFLAGS=-lboost_context -levent -L. 

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

unittest: core_test.o coroutine_test.o sceduler_test.o dispatcher_test.o event_test.o
	$(CXX) -o $@ $^ $(LDFLAGS) -lgtest -lgtest_main -lcoroutine

clean:
	rm -f main unittest *.o *.a



