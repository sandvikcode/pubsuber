add_subdirectory(src)

if (BUILD_TESTING)
  find_package(GTest CONFIG REQUIRED)

  add_subdirectory(tests)
endif()
