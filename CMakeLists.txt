cmake_minimum_required(VERSION 3.1...3.15)
if(${CMAKE_VERSION} VERSION_LESS 3.12)
  cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif()

option(WITH_TESTS "If test are build or not" OFF)

set(PACKAGE_NAME      "pubsuber")
set(PACKAGE_VERSION   "0.9.0")
set(PACKAGE_STRING    "${PACKAGE_NAME} ${PACKAGE_VERSION}")
set(PACKAGE_TARNAME   "${PACKAGE_NAME}-${PACKAGE_VERSION}")
set(PACKAGE_BUGREPORT "https://github.com/sandvikcode/pubsuber/issues")
project(${PACKAGE_NAME} VERSION ${PACKAGE_VERSION} LANGUAGES C CXX)

set(PS_INSTALL_BINDIR "bin" CACHE STRING "Installation directory for executables")
set(PS_INSTALL_LIBDIR "lib" CACHE STRING "Installation directory for libraries")
set(PS_INSTALL_INCLUDEDIR "include" CACHE STRING "Installation directory for headers")
set(PS_INSTALL_CMAKEDIR "${PS_INSTALL_LIBDIR}/${PACKAGE_NAME}/cmake" CACHE STRING "Installation directory for cmake config files")

set(PS_TARGETS_EXPORT_NAME       "${PROJECT_NAME}Targets")
set(PS_CMAKE_CONFIG_TEMPLATE     "${CMAKE_CURRENT_LIST_DIR}/cmake/config.cmake.in")
set(PS_CMAKE_GRPC_MODULE         "${CMAKE_CURRENT_LIST_DIR}/cmake/FindgRPC.cmake")
set(PS_CMAKE_CONFIG_DIR          "${CMAKE_CURRENT_BINARY_DIR}")
set(PS_CMAKE_VERSION_CONFIG_FILE "${PS_CMAKE_CONFIG_DIR}/${PROJECT_NAME}ConfigVersion.cmake")
set(PS_CMAKE_PROJECT_CONFIG_FILE "${PS_CMAKE_CONFIG_DIR}/${PROJECT_NAME}Config.cmake")
set(PS_CMAKE_PROJECT_TARGETS_FILE "${PS_CMAKE_CONFIG_DIR}/${PROJECT_NAME}Targets.cmake")

configure_file(cmake/Version.h.in PubsuberVersion.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# By default it is implied that pubsuber is build as standalone lib
set(PS_IN_SOURCE_BUILD OFF)
# CMAKE_SOURCE_DIR points to the top cmake file that may include pubsuber cmakefile
# CMAKE_CURRENT_LIST_DIR points to current pubsuber cmakefile
# If path for those two is not the same it means pubsuber is included as a subdirectory
if (NOT CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  set(PS_IN_SOURCE_BUILD ON)
  message(STATUS "Pubsuber in source build is: ${PS_IN_SOURCE_BUILD}")
endif()

set(PS_INSTALL_default ON)
if (${PS_IN_SOURCE_BUILD})
  # Disable PS_INSTALL by default if building as a submodule
  set(PS_INSTALL_default OFF)
  message(STATUS "Pubsuber install is: ${PS_INSTALL_default}")
endif()
set(PS_INSTALL ${PS_INSTALL_default} CACHE BOOL "Generate installation target")

file(TO_CMAKE_PATH "${PROJECT_BINARY_DIR}/CMakeLists.txt" LOC_PATH)
if(EXISTS "${LOC_PATH}")
    message(FATAL_ERROR "You cannot build in a source directory (or any directory with a CMakeLists.txt file). Please make a build subdirectory. Feel free to remove CMakeCache.txt and CMakeFiles.")
endif()

set(default_build_type "Release")
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
  set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE
    STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
    "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# Organaze by folders in IDE
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if (NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
else()
  if(CMAKE_CXX_STANDARD LESS 17)
    message(FATAL_ERROR "pubsuber requires at least C++17 standard")
  endif()
endif()

set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include(cmake/grpc.cmake)
include(cmake/spdlog.cmake)
if (WITH_TESTS)
  include(CTest) # Defines option BUILD_TESTING.
endif()

# Path prefix to the folder with  proto files. Required for correct in-source build
set (PS_PROTO_FILES_PATH_PREFIX ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(pubsuber)
add_subdirectory(example)
