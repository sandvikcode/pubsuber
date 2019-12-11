#!/bin/bash -vx

set -e

git clone https://github.com/gabime/spdlog.git
cd spdlog

mkdir .build && cd .build
cmake -GNinja .. && ninja install
