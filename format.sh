#!/bin/bash

ls pubsuber/src/*.cpp | xargs clang-format -i -style=file
ls pubsuber/src/*.h | xargs clang-format -i -style=file
ls pubsuber/tests/*.cpp | xargs clang-format -i -style=file
ls pubsuber/tests/*.h | xargs clang-format -i -style=file
