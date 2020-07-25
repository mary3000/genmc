#! /bin/sh

autoreconf --install

CXXFLAGS="-g -O0" CFLAGS="-g -O0" ./configure

make
