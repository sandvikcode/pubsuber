

add_library(pubsuber STATIC)

add_library(pubsuber::pubsuber ALIAS pubsuber)

target_sources(pubsuber
  PRIVATE
    Distribution.h
    Executor.cpp
    Executor.h
    MessageImpl.cpp
    MessageImpl.h
    ModAckIterator.cpp
    ModAckIterator.h
    Pubsuber.cpp
    Pubsuber.h
    SubscriptionImpl.cpp
    SubscriptionImpl.h
    SubscriptionPullIterator.cpp
    SubscriptionPullIterator.h
    TopicImpl.cpp
    TopicImpl.h
    Trimpl.cpp
    Trimpl.h
  )

set_target_properties(pubsuber PROPERTIES PUBLIC_HEADER Pubsuber.h)

protobuf_generate_grpc_cpp(GRPC_SRCS GRPC_HDRS
  ${CMAKE_SOURCE_DIR}/google/pubsub/v1/pubsub.proto
  ${CMAKE_SOURCE_DIR}/google/api/client.proto
  ${CMAKE_SOURCE_DIR}/google/api/http.proto
  ${CMAKE_SOURCE_DIR}/google/api/annotations.proto
)

target_sources(pubsuber
  PRIVATE
    ${GRPC_SRCS}
    ${GRPC_HDRS}
)

target_link_libraries(pubsuber PUBLIC gRPC::grpc++ gRPC::grpc spdlog::spdlog_header_only)

target_include_directories(pubsuber PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:include/>  # <prefix>/include
)

if(WIN32 AND MSVC)
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
