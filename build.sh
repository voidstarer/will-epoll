#!/bin/sh

if [ "$1" = "clean" ]; then
	cd lib/networking
	./clean.py
	cd ../../src
	./clean.py
	exit 0
fi

cd `dirname $0`

set -e
set -x

cd lib/networking/
cmake .
make

cd ../../src
cmake .
make
