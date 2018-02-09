TARGET = server
LDFLAGS = -lm
CC = gcc
CFLAGS = -O0 -ggdb -Wall -fno-inline

.PHONY: default all clean

default: $(TARGET) 
all: default

#OBJECTS = $(patsubst %.c, %.o, $(wildcard *.c))
OBJECTS = client.o event.o main.o log.o
HEADERS = $(wildcard *.h)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

.PRECIOUS: $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -Wall $(LDFLAGS) -o $@

test: CFLAGS+=-fprofile-arcs -ftest-coverage
test: LDFLAGS+=-fprofile-arcs -ftest-coverage
test: clean $(TARGET)
	make -C test
	rm -rf coverage
	mkdir coverage
	./test/unittest.sh
	lcov --directory . --capture --output-file coverage/app.info
	genhtml -o coverage coverage/app.info

clean:
	make -C test clean
	rm -f *.o
	rm -f $(TARGET) 
	rm -f *.gcda *.gcno
#	rm -rf coverage
