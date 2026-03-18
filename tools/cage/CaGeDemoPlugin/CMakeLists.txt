cmake_minimum_required(VERSION 3.16)
project(CaGeDemoPlugin)

find_package(metacg REQUIRED)

if(metacg_FOUND)
    message(STATUS "metacg found")
endif()

add_library(CaGeDemoPlugin SHARED CaGeDemoPlugin.cpp)

#Build metadata as separate shared object
#This .so can be preloaded to inform many graphlib tools about custom metadata
add_library(CaGeDemoMD SHARED CaGeDemoMD.h)
set_target_properties(CaGeDemoMD PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(CaGeDemoPlugin PRIVATE CaGeDemoMD)
target_link_libraries(CaGeDemoPlugin PUBLIC metacg::metacg)
