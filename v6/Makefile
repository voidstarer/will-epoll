TARGET = server client
LDFLAGS = 
CC = gcc
CXX = g++
CFLAGS = -O0 -ggdb -Wall
CPPFLAGS = $(CFLAGS) -std=gnu++11

.PHONY: default all clean

default: $(TARGET) 
all: default

#OBJECTS = $(patsubst %.cpp, %.o, $(wildcard *.cpp))
OBJECTS = client.o event.o cmgr.o packet.o log.o
HEADERS = $(wildcard *.hpp)

%.o: %.cpp $(HEADERS)
	$(CXX) $(CPPFLAGS) -c $< -o $@

.PRECIOUS: $(TARGET) $(OBJECTS) tcp_server.o server.o

server: $(OBJECTS) tcp_server.o server.o
	$(CXX) $? -Wall $(LDFLAGS) -o $@

client: $(OBJECTS) tcp_client.o my_client.o
	$(CXX) $? -Wall $(LDFLAGS) -o $@

test: CPPFLAGS+=-fprofile-arcs -ftest-coverage
test: LDFLAGS+=-fprofile-arcs -ftest-coverage
test: clean $(TARGET)
	make -C test
	rm -rf coverage
	mkdir coverage
	./test/unittest.sh
	lcov --directory . --capture --output-file coverage/app.info
	genhtml -o coverage coverage/app.info

clean:
	rm -f *.o
	rm -f $(TARGET) 
	rm -f *.gcda *.gcno
#	rm -rf coverage
