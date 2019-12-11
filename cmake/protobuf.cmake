# Defines the following variables:

# ``Protobuf_FOUND``
#   Found the Google Protocol Buffers library
#   (libprotobuf & header files)
# ``Protobuf_VERSION``
#   Version of package found.
# ``Protobuf_INCLUDE_DIRS``
#   Include directories for Google Protocol Buffers
# ``Protobuf_LIBRARIES``
#   The protobuf libraries
# ``Protobuf_PROTOC_LIBRARIES``
#   The protoc libraries
# ``Protobuf_LITE_LIBRARIES``
#   The protobuf-lite libraries

# The following :prop_tgt:`IMPORTED` targets are also defined:

# ``protobuf::libprotobuf``
#   The protobuf library.
# ``protobuf::libprotobuf-lite``
#   The protobuf lite library.
# ``protobuf::libprotoc``
#   The protoc library.
# ``protobuf::protoc``
#   The protoc compiler.

# Set to ON to force the use of the static libraries.
# Default is OFF.
set(Protobuf_USE_STATIC_LIBS ON)

find_package(Protobuf REQUIRED)

if(Protobuf_FOUND)
  # extract the include dir from target's properties
  get_target_property(PROTOBUF_WELLKNOWN_INCLUDE_DIR protobuf::libprotoc INTERFACE_INCLUDE_DIRECTORIES)

  # Define protoc compiler
  set(PROTOBUF_PROTOC protobuf::protoc)
  set(PROTOBUF_PROTOC_EXECUTABLE $<TARGET_FILE:protobuf::protoc>)

  message(STATUS "Using protobuf ${Protobuf_VERSION}")
endif()

# Sample code:
# protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS DESCRIPTORS PROTO_DESCS foo.proto)
# add_executable(bar bar.cc ${PROTO_SRCS} ${PROTO_HDRS})


