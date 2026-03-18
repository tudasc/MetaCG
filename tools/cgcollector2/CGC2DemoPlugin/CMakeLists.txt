cmake_minimum_required(VERSION 3.16)
project(CGC2DemoPlugin)

find_package(metacg REQUIRED)

if(metacg_FOUND)
    message(STATUS "metacg found")
endif()

add_library(CGC2DemoPlugin SHARED CGC2DemoPlugin.cpp)

#Build metadata as separate shared object
#This .so can be preloaded to inform many graphlib tools about custom metadata
add_library(CGC2DemoMetadata SHARED CGC2DemoMD.h)
set_target_properties(CGC2DemoMetadata PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(CGC2DemoPlugin PRIVATE CGC2DemoMetadata)
target_link_libraries(CGC2DemoPlugin PUBLIC metacg::metacg)
