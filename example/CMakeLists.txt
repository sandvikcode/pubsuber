# set(CMAKE_DEBUG_TARGET_PROPERTIES
#   INCLUDE_DIRECTORIES
#   COMPILE_DEFINITIONS
#   COMPILE_FEATURES
#   COMPILE_OPTIONS
#   LINK_DIRECTORIES
# )

add_executable(client client.cpp)

target_link_libraries(client PRIVATE pubsuber::pubsuber)
