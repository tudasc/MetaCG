# Cube library installation
find_library(CUBE_LIB cube4 PATHS ${CUBE_DIR}/lib REQUIRED)
find_path(CUBE_INCLUDE Cube.h PATHS ${CUBE_DIR}/include/cubelib REQUIRED)

message(STATUS "Using cubelib: ${CUBE_LIB}")
message(STATUS "Using cube headers: ${CUBE_INCLUDE}")

function(add_cube target)
  target_include_directories(${target} SYSTEM PUBLIC ${CUBE_INCLUDE})
  target_link_libraries(${target} PUBLIC ${CUBE_LIB})
endfunction()
