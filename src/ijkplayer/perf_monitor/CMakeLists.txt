cmake_minimum_required(VERSION 3.22.1)
project("perfmonitor" LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

set(PERF_MONITOR_LIB_NAME "perfmonitor")


add_library(${PERF_MONITOR_LIB_NAME}
    src/perf_monitor.cpp
)

target_include_directories(${PERF_MONITOR_LIB_NAME}
    PRIVATE ./src/inc
    PRIVATE ../../inc
    PUBLIC ./include
)

