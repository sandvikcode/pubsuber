#!/bin/bash -vx

set -e

git clone -b v1.25.0 https://github.com/grpc/grpc
cd grpc
git submodule update --init

mkdir -p cmake/build
cd cmake/build

cmake -GNinja -DgRPC_ZLIB_PROVIDER=package -DgRPC_CARES_PROVIDER=package -DgRPC_SSL_PROVIDER=package -DgRPC_PROTOBUF_PROVIDER=package -DCMAKE_BUILD_TYPE=Release ../..

ninja -v
ninja install
ldconfig
