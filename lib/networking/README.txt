Networking Library
written on pure C++11



Information

========================================================================

Input/Output schedulling implemented with epoll/kqueue.
Frame - network object with fixed size. Frames can be service or data typed.





Requirements

========================================================================

g++ 4.8+
openssl 1.0.1+
cmake 2.8+
operating system Linux / MacOS / FreeBSD



Building

========================================================================

Compilinng as a static library:

cd networking
cmake .
make


after success compilation you can find static library located at lib/libnetworking.a
