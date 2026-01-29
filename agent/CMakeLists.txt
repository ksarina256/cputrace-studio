cmake_minimum_required(VERSION 3.20)
project(cputrace_agent CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(cputrace src/main.cpp)

target_compile_options(cputrace PRIVATE -Wall -Wextra -Wpedantic -g)

add_executable(perf_hello
  src/perf_hello.cpp
)
target_compile_options(perf_hello PRIVATE -Wall -Wextra -Wpedantic -g)
