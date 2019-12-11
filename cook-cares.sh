#!/bin/bash -vx

set -e

git clone -b cares-1_15_0 https://github.com/c-ares/c-ares.git
cd c-ares

mkdir -p build
cd build

cmake -GNinja -DCARES_STATIC=ON -DCARES_SHARED=OFF -DCARES_STATIC_PIC=ON -DCMAKE_BUILD_TYPE=Release  ..

# -DCMAKE_INSTALL_PREFIX=/usr/local/cares

ninja -v
ninja install
ldconfig
