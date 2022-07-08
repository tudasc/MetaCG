# Extra-P installation directory for library
find_library(EXTRAP_LIB extrap)
# Extra-P source tree for includes (they are not installed with extra-p)
find_path(EXTRAP_INCLUDE extrap)

function(add_extrap target)
  target_include_directories(${target} SYSTEM PUBLIC ${EXTRAP_INCLUDE})
  target_link_directories(
    ${target}
    PUBLIC
    ${EXTRAP_LIB}
  )
  target_link_libraries(${target} extrap)
endfunction()
