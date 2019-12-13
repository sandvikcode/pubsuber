add_library(pubsuber STATIC)

add_library(pubsuber::pubsuber ALIAS pubsuber)

target_sources(pubsuber
  PRIVATE
    src/Distribution.h
    src/Executor.cpp
    src/Executor.h
    src/MessageImpl.cpp
    src/MessageImpl.h
    src/ModAckIterator.cpp
    src/ModAckIterator.h
    src/Pubsuber.cpp
    include/pubsuber/Pubsuber.h
    src/SubscriptionImpl.cpp
    src/SubscriptionImpl.h
    src/SubscriptionPullIterator.cpp
    src/SubscriptionPullIterator.h
    src/TopicImpl.cpp
    src/TopicImpl.h
    src/Trimpl.cpp
    src/Trimpl.h
  )

set_target_properties(pubsuber PROPERTIES PUBLIC_HEADER include/pubsuber/Pubsuber.h)

protobuf_generate_grpc_cpp(GRPC_SRCS GRPC_HDRS PROTO_PATH_PREFIX ${PS_PROTO_FILES_PATH_PREFIX}
  PROTO_FILES
  ${PS_PROTO_FILES_PATH_PREFIX}/google/pubsub/v1/pubsub.proto
  ${PS_PROTO_FILES_PATH_PREFIX}/google/api/client.proto
  ${PS_PROTO_FILES_PATH_PREFIX}/google/api/http.proto
  ${PS_PROTO_FILES_PATH_PREFIX}/google/api/annotations.proto
)

target_sources(pubsuber
  PRIVATE
    ${GRPC_SRCS}
    ${GRPC_HDRS}
)

target_link_libraries(pubsuber PUBLIC gRPC::grpc++ gRPC::grpc spdlog::spdlog_header_only)

target_include_directories(pubsuber PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
  $<INSTALL_INTERFACE:include/>  # <prefix>/include
)

if(WIN32 AND MSVC)
  target_compile_definitions(pubsuber PRIVATE
    -D_WIN32_WINNT=0x600
    -D_SCL_SECURE_NO_WARNINGS
    -D_CRT_SECURE_NO_WARNINGS
    -D_WINSOCK_DEPRECATED_NO_WARNINGS
  )

  set_target_properties(pubsuber PROPERTIES COMPILE_PDB_NAME "pubsuber"
    COMPILE_PDB_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
  )
  if (PS_INSTALL)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/pubsuber.pdb
      DESTINATION ${PS_INSTALL_LIBDIR} OPTIONAL
    )
  endif()
endif()


# INSTALL
# install header files, generate and install cmake config files for find_package()

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  ${PS_CMAKE_VERSION_CONFIG_FILE}
  VERSION ${PACKAGE_VERSION}
  COMPATIBILITY SameMajorVersion
)

include(CMakePackageConfigHelpers)
configure_package_config_file(${PS_CMAKE_CONFIG_TEMPLATE}
  ${PS_CMAKE_PROJECT_CONFIG_FILE}
  INSTALL_DESTINATION ${PS_INSTALL_CMAKEDIR}
  PATH_VARS
    PS_INSTALL_LIBDIR
    PS_INSTALL_INCLUDEDIR
    PS_INSTALL_CMAKEDIR
)

if (PS_INSTALL)
  install(TARGETS pubsuber EXPORT ${PS_TARGETS_EXPORT_NAME}
    PUBLIC_HEADER DESTINATION "${PS_INSTALL_INCLUDEDIR}/${PACKAGE_NAME}"
    RUNTIME DESTINATION ${PS_INSTALL_BINDIR}
    LIBRARY DESTINATION ${PS_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${PS_INSTALL_LIBDIR}
  )

  install(FILES ${PS_CMAKE_PROJECT_CONFIG_FILE} ${PS_CMAKE_VERSION_CONFIG_FILE} ${PS_CMAKE_GRPC_MODULE}
    DESTINATION ${PS_INSTALL_CMAKEDIR}
  )

  install(EXPORT ${PS_TARGETS_EXPORT_NAME}
    DESTINATION ${PS_INSTALL_CMAKEDIR}
    NAMESPACE ${PROJECT_NAME}::
  )

endif()


if (BUILD_TESTING)
  find_package(GTest CONFIG REQUIRED)

  add_subdirectory(tests)
endif()
