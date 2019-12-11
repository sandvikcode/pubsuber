# GTest::gtest GTest::gtest_main GTest::gmock GTest::gmock_main

add_executable(pubsuber-tests FunctionsTest.cpp)

target_link_libraries(pubsuber-tests pubsuber::pubsuber GTest::gtest GTest::gtest_main)

add_test(NAME AllTests COMMAND pubsuber-tests)
