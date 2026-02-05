file(
  GLOB
  SOURCES
  "*.f90"
)
add_executable(02 ${SOURCES})

set_target_properties(02 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
