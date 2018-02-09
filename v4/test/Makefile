TARGET = simpleclient
LDFLAGS = -lm
CC = gcc
CFLAGS = -O0 -ggdb -Wall -fno-inline

.PHONY: default all clean

default: $(TARGET)
all: default

OBJECTS = simpleclient.o
HEADERS = $(wildcard *.h)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

.PRECIOUS: $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -Wall $(LDFLAGS) -o $@

clean:
	rm -f *.o
	rm -f $(TARGET) simpleclient
