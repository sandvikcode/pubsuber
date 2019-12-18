# GTest::gtest GTest::gtest_main GTest::gmock GTest::gmock_main

add_executable(pubsuber-tests TestMain.cpp FunctionsTest.cpp RetriableTest.cpp ExpoBackoffTest.cpp)

target_link_libraries(pubsuber-tests pubsuber::pubsuber GTest::gtest GTest::gmock)

add_test(NAME AllTests COMMAND pubsuber-tests)
