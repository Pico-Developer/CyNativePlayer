
cmake_minimum_required(VERSION 3.22.1)

project(nativeplayerdemo)

add_library(nativeplayerdemo SHARED src/main.cpp)

target_include_directories(nativeplayerdemo PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_directories(nativeplayerdemo PRIVATE ${CMAKE_SOURCE_DIR}/lib/${ANDROID_ABI})
target_link_libraries(nativeplayerdemo PRIVATE nativeplayer)
