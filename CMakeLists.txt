cmake_minimum_required(VERSION 3.15)
project(obfuscxx VERSION 1.0.0 LANGUAGES CXX)

## message(STATUS "=========================================")
## message(STATUS "C Compiler: ${CMAKE_C_COMPILER}")
## message(STATUS "C++ Compiler: ${CMAKE_CXX_COMPILER}")
## message(STATUS "Compiler ID: ${CMAKE_CXX_COMPILER_ID}")
## message(STATUS "Compiler Version: ${CMAKE_CXX_COMPILER_VERSION}")
## message(STATUS "=========================================")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

find_package(GTest CONFIG REQUIRED)
find_package(benchmark CONFIG REQUIRED)

add_executable(obfuscxx_test obfuscxx/quick_test.cpp)

target_link_libraries(obfuscxx_test PRIVATE
    GTest::gtest
    GTest::gtest_main
    benchmark::benchmark
    benchmark::benchmark_main
    Threads::Threads
)
target_include_directories(obfuscxx_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/obfuscxx
)

enable_testing()
add_test(NAME obfuscxx_test COMMAND obfuscxx_test)

include(GoogleTest)
## gtest_discover_tests(obfuscxx_test)