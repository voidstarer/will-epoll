TARGET = server
LDFLAGS = 
CC = gcc
CXX = g++
CFLAGS = -O0 -ggdb -Wall
CPPFLAGS = $(CFLAGS) -std=gnu++11

.PHONY: default all clean

default: $(TARGET) 
all: default

OBJECTS = $(patsubst %.cpp, %.o, $(wildcard *.cpp))
HEADERS = $(wildcard *.hpp)

%.o: %.cpp $(HEADERS)
	$(CXX) $(CPPFLAGS) -c $< -o $@

.PRECIOUS: $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -Wall $(LDFLAGS) -o $@

clean:
	rm -f *.o
	rm -f $(TARGET) 
	rm -f *.gcda *.gcno
#	rm -rf coverage
