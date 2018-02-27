#!/bin/sh -x

cd `dirname $0`
cd ..
./server -h > /dev/null 2>&1
./server -l 0 -p 9000 > /tmp/server.log 2>&1 &
SERVER_PID=$!
sleep 1
cd -
echo Running server with pid $SERVER_PID
#./simpleclient -r -s 127.0.0.1 -i 2 -d 1 > /dev/null 2>&1 &
../client -s 127.0.0.1 -i 2 -d 1 -l 2 > /dev/null 2>&1 &
SCR_PID=$!
./simpleclient -s 127.0.0.1 -i 1 -d 2 > /dev/null 2>&1
# > /dev/null 2>&1
kill -INT $SCR_PID
sleep 1
kill -INT $SERVER_PID
wait
