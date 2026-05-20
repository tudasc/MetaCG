cmake_minimum_required(VERSION 3.20)

include(${CGFCOLLECTOR_TEST_CMAKE_BASE})

project(operator LANGUAGES Fortran)

file(
  GLOB
  SOURCES
  "*.f90"
)

add_executable(${CMAKE_PROJECT_NAME} ${SOURCES})
