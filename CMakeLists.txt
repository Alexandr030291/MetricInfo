cmake_minimum_required(VERSION 3.6)

project(MetricInfo)

link_libraries(-lcurl)
link_libraries(-lpthread)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES main.cpp info.h info.cpp)
add_executable(MetricInfo ${SOURCE_FILES})

