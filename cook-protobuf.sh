#!/bin/bash -vx

set -e

git clone --depth 1 https://github.com/protocolbuffers/protobuf.git
cd protobuf
git fetch -t
git checkout v3.11.0
git submodule update --init --recursive
./autogen.sh
./configure
make -j8
make check
make install
ldconfig # refresh shared library cache.
