cmake_minimum_required(VERSION 3.20)

include(../cmake_base.txt)

project(module_inherit LANGUAGES Fortran)

file(
  GLOB
  SOURCES
  "*.f90"
)

add_executable(${CMAKE_PROJECT_NAME} ${SOURCES})
