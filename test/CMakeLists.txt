cmake_minimum_required(VERSION 3.13)

include(FetchContent)

#FetchContent_Declare(
#  googletest
#  URL https://github.com/google/googletest/archive/refs/tags/release-1.12.0.zip
#)
#FetchContent_MakeAvailable(googletest)
include(GoogleTest)

set(TEST_NAME myplan-test)

file(GLOB testSRC
    "*.cpp"
)
set(TEST_RUNNER main.cpp ${testSRC})
add_executable(${TEST_NAME} ${TEST_RUNNER})

target_link_libraries(${TEST_NAME} gtest gtest_main pthread libmyplan)

gtest_discover_tests(${TEST_NAME})
