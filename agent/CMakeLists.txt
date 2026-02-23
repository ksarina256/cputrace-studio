cmake_minimum_required(VERSION 3.16)
project(CPUTrace VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

include_directories(src)

add_executable(cputrace
  src/main.cpp
  src/proc_status.cpp
  src/proc_stat.cpp
  src/perf_counter.cpp
  src/symbol_resolver.cpp
)

if(NOT TARGET perf_hello)
  add_executable(perf_hello
    src/perf_hello.cpp
  )
endif()

add_executable(test_app
  src/test_app.cpp
)
target_compile_options(test_app PRIVATE -g -O0)
