# pubsuber
C++, GRPC based client for Google PubSub.
Why? Simply because Google does not provide one.


## Table of Contents
- [Requirements](#requirements)
  - [Compiler](#compiler)
  - [Build Tools](#build-tools)
  - [Libraries](#libraries)
  - [Tests](#tests)
- [Install Dependencies](#install-dependencies)
- [Build](#build)
  - [Linux](#linux)
  - [macOS](#macOS)
  - [Windows](#windows)
- [Install](#installing-pubsuber-using-cmake)
- [Versioning](#versioning)
- [Contributing changes](#contributing-changes)
- [Licensing](#licensing)

## Requirements

#### Compiler

Pubsuber is tested with the following compilers:

| Compiler    | Minimum Version |
| ----------- | --------------- |
| GCC         | 9.0 |
| Clang       | 9.0.0 |
| MSVC++      | 16.4 |
| Apple Clang | 11.0.0 |

#### Build Tools

Pubsuber can be built with [CMake](https://cmake.org).  The minimal versions of cmake is 3.15

#### Libraries

Pubsuber also depend on gRPC, protobuf, and google API proto files (committed in the repo).
Pubsuber is tested with the following versions of these dependencies:

| Library  | Minimum version | Notes |
| -------- | --------------- | ----- |
| gRPC     | v1.24.x | |
| protobuf | v3.10.1 | |
| spdlog   | v1.4.2  | |

#### Tests

TODO:
Integration tests at times use the Google pubsub emulator.
The integration tests run against the latest version of the SDK on each commit and PR.

## Install Dependencies

First install the development tools.
Second install required dependencies.
Its C++ land and its up to the developer to decide how to install the dependencies.


## Build

To build all available libraries and run the tests, run the following commands after cloning this repo:

#### Linux

```bash
# Add -DBUILD_TESTING=OFF to disable tests
mkdir .build
cd .build
cmake ..

# Adjust the number of threads used by modifying parameter for `-j 4`
# following command will also invoke ctest at the end
cmake --build cmake-out -- -j 4

# Verify build by running tests
ctest --output-on-failure
```

#### macOS

```bash
mkdir .build
cd .build
cmake ..

# Adjust the number of threads used by modifying parameter for `-j 4`
cmake --build cmake-out -- -j 4

# Verify build by running tests
ctest --output-on-failure
```

#### Windows

```bash
mkdir .build
cd .build
cmake ..

Open visual studio solution and build it
```

### Installing `pubsuber` using CMake

The default CMake builds for `pubsuber` assume that all the necessary
dependencies are installed in your system. Installing the dependencies may be as simple as using the package manager for your platform, or may require manually downloading, compiling, and installing a number of additional libraries.

Assuming `~/install-test` is exist one can run follwing commands from the repo root:

```bash
mkdir .build && cd .build
cmake -DCMAKE_INSTALL_PREFIX=~/install-test ..
cmake -j4 install
```


## Versioning

Pubsuber client follows [Semantic Versioning](http://semver.org/).

## Contributing changes

See [`CONTRIBUTING.md`](CONTRIBUTING.md) for details on how to contribute to
this project, including how to build and test your changes as well as how to
properly format your code.

## Licensing

MIT; see [`LICENSE`](LICENSE) for details.
