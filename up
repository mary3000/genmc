#! /bin/sh

autoreconf --install

# mac-specific
CXXFLAGS="-g -O0" CFLAGS="-g -O0" ./configure AR=llvm-ar --with-llvm=/usr/local/opt/llvm@9 --with-clang=/usr/local/opt/llvm@9/bin/clang --with-clangxx=/usr/local/opt/llvm@9/bin/clang++

make install
