#!/bin/bash

find pubsuber -type f -name *.h -print0 | xargs -0 clang-format -i -style=file
find pubsuber -type f -name *.cpp -print0 | xargs -0 clang-format -i -style=file
