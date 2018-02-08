TARGET = server
LIBS = -lm
CC = gcc
CFLAGS = -O0 -ggdb -Wall -fno-inline

.PHONY: default all clean

default: $(TARGET) simpleclient
all: default

#OBJECTS = $(patsubst %.c, %.o, $(wildcard *.c))
OBJECTS = client.o event.o main.o log.o
HEADERS = $(wildcard *.h)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

.PRECIOUS: $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -Wall $(LIBS) -o $@

simpleclient: simpleclient.c
	$(CC) simpleclient.c -Wall $(LIBS) -o $@


clean:
	rm -f *.o
	rm -f $(TARGET) simpleclient
