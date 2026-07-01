cmake_minimum_required(VERSION 3.10.2)
project("perfmonitor" LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

set(PERF_MONITOR_LIB_NAME "perfmonitor")

add_library(${PERF_MONITOR_LIB_NAME}
    src/perf_monitor.cpp
)

set_target_properties(${PERF_MONITOR_LIB_NAME} PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)

target_include_directories(${PERF_MONITOR_LIB_NAME}
    PRIVATE ./src/inc
    PRIVATE ../../inc
    PUBLIC ./include
)

target_link_libraries(${PERF_MONITOR_LIB_NAME}
    PRIVATE log
)
