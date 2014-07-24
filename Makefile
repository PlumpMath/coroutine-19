
CPPFLAGS=-Wall -I. -ggdb
LDFLAGS=-lboost_context

CC=gcc
CXX=g++

%.o: %.c
	$(CC) -c -o $@ $(CPPFLAGS) $<

%.o: %.cpp
	$(CXX) -c -o $@ $(CPPFLAGS) $<

all: main

main:main.o core.o
	$(CXX) -o $@ $^ $(LDFLAGS)

# ------ unit test ------

test: core_test

core_test: core_test.o
	$(CXX) -o $@ $^ $(LDFLAGS) -lgtest -lgtest_main

clean:
	rm -f main core_test *.o



