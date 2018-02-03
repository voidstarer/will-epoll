#!/bin/sh

cd `dirname $0`

set -e
set -x

cd lib/networking/
cmake .
make

cd ../../src
cmake .
make
