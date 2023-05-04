# Cube library installation
find_library(CUBE_LIB cube4)
find_path(CUBE_INCLUDE cube)

function(add_cube target)
  target_include_directories(${target} SYSTEM PUBLIC ${CUBE_INCLUDE})
  target_link_directories(
    ${target}
    PUBLIC
    ${CUBE_LIB}
  )
  target_link_libraries(${target} PUBLIC cube4)
endfunction()
