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


add_executable(obfuscxx_test obfuscxx/quick_test.cpp)

target_include_directories(obfuscxx_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/obfuscxx
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(obfuscxx_test PRIVATE -Wall -Wextra)
elseif(MSVC)
    target_compile_options(obfuscxx_test PRIVATE /W4)
endif()

