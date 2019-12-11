# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

#[=======================================================================[.rst:
FindgRPC
------------

Locate and configure the Google Protocol Buffers library.

The following variables can be set and are optional:

``gRPC_SRC_ROOT_FOLDER``
  When compiling with MSVC, if this cache variable is set
  the grpc-default VS project build locations
  (vsprojects/Debug and vsprojects/Release
  or vsprojects/x64/Debug and vsprojects/x64/Release)
  will be searched for libraries and binaries.
``gRPC_IMPORT_DIRS``
  List of additional directories to be searched for
  imported .proto files.
``gRPC_DEBUG``
  Show debug messages.
``gRPC_USE_STATIC_LIBS``
  Set to ON to force the use of the static libraries.
  Default is OFF.

Defines the following variables:

``gRPC_FOUND``
  Found the Google RPC
  (lib & header files)
``gRPC_VERSION``
  Version of package found.
``gRPC_INCLUDE_DIRS``
  Include directories for Google Protocol Buffers
``gRPC_LIBRARIES``
  The grpc libraries

The following :prop_tgt:`IMPORTED` targets are also defined:

``gRPC::grpc++``
  The gRPC library.
``gRPC::grpc_cpp_plugin``
  The protoc cpp compiler plugin.

The following cache variables are also available to set or use:

`gRPC_LIBRARY``
  The grpc library
``gRPC_INCLUDE_DIR``
  The include directory for grpc
``gRPC_CPP_PLUGIN_EXECUTABLE``
  The protoc compiler cpp plugin
``gRPC_LIBRARY_DEBUG``
  The grpc library (debug)

#]=======================================================================]

if(gRPC_DEBUG)
  # Output some of their choices
  message(STATUS "[ ${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE} ] "
                 "gRPC_USE_STATIC_LIBS = ${gRPC_USE_STATIC_LIBS}")
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(_gRPC_ARCH_DIR x64/)
endif()


# Support preference of static libs by adjusting CMAKE_FIND_LIBRARY_SUFFIXES
if( gRPC_USE_STATIC_LIBS )
  set( _grpc_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES})
  if(WIN32)
    set(CMAKE_FIND_LIBRARY_SUFFIXES .lib .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
  else()
    set(CMAKE_FIND_LIBRARY_SUFFIXES .a )
  endif()
endif()

include(SelectLibraryConfigurations)

# Internal function: search for normal library as well as a debug one
#    if the debug one is specified also include debug/optimized keywords
#    in *_LIBRARIES variable
function(_grpc_find_libraries name filename)
  if(${name}_LIBRARIES)
    # Use result recorded by a previous call.
    return()
  elseif(${name}_LIBRARY)
    # Honor cache entry used by CMake 3.5 and lower.
    set(${name}_LIBRARIES "${${name}_LIBRARY}" PARENT_SCOPE)
  else()
    find_library(${name}_LIBRARY_RELEASE
      NAMES ${filename}
      PATHS ${gRPC_SRC_ROOT_FOLDER}/vsprojects/${_gRPC_ARCH_DIR}Release)
    mark_as_advanced(${name}_LIBRARY_RELEASE)

    find_library(${name}_LIBRARY_DEBUG
      NAMES ${filename}d ${filename}
      PATHS ${gRPC_SRC_ROOT_FOLDER}/vsprojects/${_gRPC_ARCH_DIR}Debug)
    mark_as_advanced(${name}_LIBRARY_DEBUG)

    select_library_configurations(${name})

    if(UNIX AND Threads_FOUND)
      list(APPEND ${name}_LIBRARIES ${CMAKE_THREAD_LIBS_INIT})
    endif()

    set(${name}_LIBRARY "${${name}_LIBRARY}" PARENT_SCOPE)
    set(${name}_LIBRARIES "${${name}_LIBRARIES}" PARENT_SCOPE)
  endif()
endfunction()

#
# Main.
#

# Google's provided vcproj files generate libraries with a "lib"
# prefix on Windows
if(MSVC)
    set(gRPC_ORIG_FIND_LIBRARY_PREFIXES "${CMAKE_FIND_LIBRARY_PREFIXES}")
    set(CMAKE_FIND_LIBRARY_PREFIXES "lib" "")
endif()

if(UNIX)
  # gRPC headers may depend on threading.
  find_package(Threads QUIET)
endif()

# The gRPC library
_grpc_find_libraries(gRPC_C grpc)

_grpc_find_libraries(gRPC grpc++)

if(gRPC_DEBUG)
  # Output some of their choices
  message(STATUS "[ ${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE} ] "
                 "gRPC_LIBRARY = ${gRPC_LIBRARY}")
endif()

# Restore original find library prefixes
if(MSVC)
    set(CMAKE_FIND_LIBRARY_PREFIXES "${gRPC_ORIG_FIND_LIBRARY_PREFIXES}")
endif()


# Find the include directory
find_path(gRPC_INCLUDE_DIR
    grpc++/grpc++.h
    PATHS ${gRPC_SRC_ROOT_FOLDER}/src
)
mark_as_advanced(gRPC_INCLUDE_DIR)


# Find the grpc_cpp_plugin Executable
find_program(gRPC_CPP_PLUGIN_EXECUTABLE
    NAMES grpc_cpp_plugin
    DOC "The Google Protocol Buffers Compiler CPP plugin"
    PATHS
    ${gRPC_SRC_ROOT_FOLDER}/vsprojects/${_gRPC_ARCH_DIR}Release
    ${gRPC_SRC_ROOT_FOLDER}/vsprojects/${_gRPC_ARCH_DIR}Debug
)
mark_as_advanced(gRPC_CPP_PLUGIN_EXECUTABLE)

if(gRPC_DEBUG)
    message(STATUS "[ ${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE} ] "
        "requested version of gRPC is ${gRPC_FIND_VERSION}")
endif()

if(gRPC_INCLUDE_DIR)
  set(gRPC_VERSION "unknown")

  if(gRPC_LIBRARY AND gRPC_C_LIBRARY)
      if(NOT TARGET gRPC::grpc++)
          add_library(gRPC::grpc++ UNKNOWN IMPORTED)
          set_target_properties(gRPC::grpc++ PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${gRPC_INCLUDE_DIR}")
          if(EXISTS "${gRPC_LIBRARY}")
            set_target_properties(gRPC::grpc++ PROPERTIES
              IMPORTED_LOCATION "${gRPC_LIBRARY}")
          endif()

          add_library(gRPC::grpc UNKNOWN IMPORTED)
          if(EXISTS "${gRPC_C_LIBRARY}")
            set_target_properties(gRPC::grpc PROPERTIES
              IMPORTED_LOCATION "${gRPC_C_LIBRARY}")
          endif()
          set_target_properties(gRPC::grpc PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${gRPC_INCLUDE_DIR}")

          if(EXISTS "${gRPC_LIBRARY_RELEASE}")
            set_property(TARGET gRPC::grpc++ APPEND PROPERTY
              IMPORTED_CONFIGURATIONS RELEASE)
            set_target_properties(gRPC::grpc++ PROPERTIES
              IMPORTED_LOCATION_RELEASE "${gRPC_LIBRARY_RELEASE}")
          endif()
          if(EXISTS "${gRPC_LIBRARY_DEBUG}")
            set_property(TARGET gRPC::grpc++ APPEND PROPERTY
              IMPORTED_CONFIGURATIONS DEBUG)
            set_target_properties(gRPC::grpc++ PROPERTIES
              IMPORTED_LOCATION_DEBUG "${gRPC_LIBRARY_DEBUG}")
          endif()
          if(UNIX AND TARGET Threads::Threads)
            set_property(TARGET gRPC::grpc++ APPEND PROPERTY
                INTERFACE_LINK_LIBRARIES Threads::Threads)
          endif()
          if(TARGET protobuf::libprotobuf)
            set_property(TARGET gRPC::grpc++ APPEND PROPERTY
                INTERFACE_LINK_LIBRARIES protobuf::libprotobuf)
          endif()
      endif()
  endif()

  if(gRPC_CPP_PLUGIN_EXECUTABLE)
      if(NOT TARGET gRPC::grpc_cpp_plugin)
          add_executable(gRPC::grpc_cpp_plugin IMPORTED)
          if(EXISTS "${gRPC_CPP_PLUGIN_EXECUTABLE}")
            set_target_properties(gRPC::grpc_cpp_plugin PROPERTIES
              IMPORTED_LOCATION "${gRPC_CPP_PLUGIN_EXECUTABLE}")
          endif()
      endif()
  endif()
endif()

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(gRPC
    REQUIRED_VARS gRPC_LIBRARIES gRPC_INCLUDE_DIR
    VERSION_VAR gRPC_VERSION
)

if(gRPC_FOUND)
    set(gRPC_INCLUDE_DIRS ${gRPC_INCLUDE_DIR})
endif()

# Restore the original find library ordering
if( gRPC_USE_STATIC_LIBS )
  set(CMAKE_FIND_LIBRARY_SUFFIXES ${_grpc_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES})
endif()

