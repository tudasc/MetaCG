cmake_minimum_required(VERSION 3.20)

project(deps LANGUAGES Fortran)

include(../cmake_base.txt)

file(
  GLOB
  SOURCES
  "*.f90"
)

add_executable(${CMAKE_PROJECT_NAME} ${SOURCES})
