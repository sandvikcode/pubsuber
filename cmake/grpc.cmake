include(cmake/protobuf.cmake)

set(gRPC_USE_STATIC_LIBS ON)
# Find gRPC installation using normal config way
find_package(gRPC CONFIG)
if (NOT gRPC_FOUND)
  message(WARNING "gRPC not found using CONFIG, trying module approach using home-grown module")

  #set(gRPC_DEBUG ON)
  # Use weird NIH module way
  find_package(gRPC REQUIRED)
  find_package(ZLIB REQUIRED)
  find_package(OpenSSL REQUIRED)

  # Wild hack bcuz gRPC doesn not provide CONFIG in packages
  set_property(TARGET gRPC::grpc++ APPEND PROPERTY
      INTERFACE_LINK_LIBRARIES ZLIB::ZLIB OpenSSL::SSL cares)

  set_property(TARGET gRPC::grpc++ APPEND PROPERTY
      INTERFACE_LINK_DIRECTORIES /usr/lib /opt/local/lib)

endif()

# finally found ?
if (gRPC_FOUND)
  message(STATUS "Using gRPC ${gRPC_VERSION}")

  set(GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:gRPC::grpc_cpp_plugin>)
endif()

set(PROTO_GENS_DIR ${CMAKE_CURRENT_BINARY_DIR}/gens)
file(MAKE_DIRECTORY ${PROTO_GENS_DIR})
# make it generaly available
include_directories(${PROTO_GENS_DIR})

# protobuf_generate_grpc_cpp (<SRCS> <HDRS> PROTO_PATH_PREFIX prefix PROTO_FILES [<ARGN>...])
#  --------------------------
#
#   Add custom commands to process ``.proto`` files to C++ using protoc and
#   GRPC plugin::
#
#     protobuf_generate_grpc_cpp [<ARGN>...]
#
#   ``ARGN``
#     ``.proto`` files
#

function(protobuf_generate_grpc_cpp SRCS HDRS)
  cmake_parse_arguments(protobuf_generate_grpc_cpp "" "PROTO_PATH_PREFIX" "PROTO_FILES"  ${ARGN})

  set(_proto_prefix "${protobuf_generate_grpc_cpp_PROTO_PATH_PREFIX}")
  if(NOT _proto_prefix)
    message(SEND_ERROR "Error: PROTOBUF_GENERATE_GRPC_CPP() called without PROTO_PATH_PREFIX")
    return()
  endif()

  set(_proto_files "${protobuf_generate_grpc_cpp_PROTO_FILES}")
  if(NOT _proto_files)
    message(SEND_ERROR "Error: PROTOBUF_GENERATE_GRPC_CPP() called without any proto files")
    return()
  endif()

  set(SRCS_OUT)
  set(HDRS_OUT)

  set(_protobuf_include_path -I ${_proto_prefix} -I ${Protobuf_INCLUDE_DIRS})
  foreach(FIL ${_proto_files})
    get_filename_component(ABS_FIL ${FIL} ABSOLUTE)
    get_filename_component(FIL_WE ${FIL} NAME_WE)
    file(RELATIVE_PATH REL_FIL ${_proto_prefix} ${ABS_FIL})
    get_filename_component(REL_DIR ${REL_FIL} DIRECTORY)
    set(RELFIL_WE "${REL_DIR}/${FIL_WE}")

    set(FIL_LIST
      "${PROTO_GENS_DIR}/${RELFIL_WE}.grpc.pb.cc"
      "${PROTO_GENS_DIR}/${RELFIL_WE}.grpc.pb.h"
      "${PROTO_GENS_DIR}/${RELFIL_WE}_mock.grpc.pb.h"
      "${PROTO_GENS_DIR}/${RELFIL_WE}.pb.cc"
      "${PROTO_GENS_DIR}/${RELFIL_WE}.pb.h"
    )

    add_custom_command(
      OUTPUT ${FIL_LIST}
      COMMAND ${PROTOBUF_PROTOC_EXECUTABLE}
      ARGS --grpc_out=generate_mock_code=true:${PROTO_GENS_DIR}
           --cpp_out=${PROTO_GENS_DIR}
           --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN_EXECUTABLE}
           ${_protobuf_include_path}
           ${REL_FIL}
      DEPENDS ${ABS_FIL} ${PROTOBUF_PROTOC} ${GRPC_CPP_PLUGIN_EXECUTABLE}
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      COMMENT "Running gRPC C++ protocol buffer compiler on ${FIL}"
      VERBATIM)

    set_source_files_properties(${FIL_LIST} PROPERTIES GENERATED TRUE)

    foreach(_file ${FIL_LIST})
      if(_file MATCHES "cc$")
        list(APPEND SRCS_OUT ${_file})
      else()
        list(APPEND HDRS_OUT ${_file})
      endif()
    endforeach()
  endforeach()

  set(${SRCS} ${SRCS_OUT} PARENT_SCOPE)
  set(${HDRS} ${HDRS_OUT} PARENT_SCOPE)
endfunction()
