#!/bin/bash -vx

set -e

# This file supposed to be run from /pubsuber folder in the container

mkdir .build && cd .build

cmake -GNinja ..

ninja -v
