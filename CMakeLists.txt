cmake_minimum_required(VERSION 3.31)
project(SmallBasicLLVM)

set(CMAKE_CXX_STANDARD 20)

find_package(spdlog REQUIRED)
find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

find_path(CXXOPTS_INCLUDE_DIR cxxopts.hpp)

if(NOT EXISTS "${CXXOPTS_INCLUDE_DIR}/cxxopts.hpp")
    message(FATAL_ERROR "Could not find cxxopts")
endif()

message(STATUS "Using cxxopts.hpp from: ${CXXOPTS_INCLUDE_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

execute_process(
        COMMAND llvm-config --libs all
        OUTPUT_VARIABLE llvm_libs
        OUTPUT_STRIP_TRAILING_WHITESPACE
)

add_executable(SmallBasicCompiler src/main.cpp
        src/lexer/lexer.cpp
        src/parser/parser.cpp
        src/parser/ast.cpp
        src/semantic/semantic.cpp
        src/codegen/codegen.cpp
        src/linker/linker.cpp)

target_compile_definitions(SmallBasicCompiler PRIVATE VERSION="0.9.4")
target_link_libraries(SmallBasicCompiler ${llvm_libs} spdlog::spdlog)

add_library(SmallBasicLibrary STATIC
        src/std/main.cpp
        src/std/value.cpp
        src/std/array.cpp
        src/std/textwindow.cpp
        src/std/clock.cpp
        src/std/math.cpp
        src/std/program.cpp
)