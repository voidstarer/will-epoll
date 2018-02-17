#!/bin/sh -x

cd `dirname $0`
cd ..
./server -h > /dev/null 2>&1
./server -l 2 -p 2222 > /dev/null 2>&1 &
SERVER_PID=$!
cd -
echo Running server with pid $SERVER_PID
./simpleclient -r -s 127.0.0.1 -i 2 -d 1 > /dev/null 2>&1 &
SCR_PID=$!
./simpleclient -s 127.0.0.1 -i 1 -d 2  > /dev/null 2>&1
sleep 5
kill -INT $SCR_PID
sleep 1
kill -INT $SERVER_PID
wait
