include(ExternalProject)

if(DEFINED CXXOPTS_INCLUDE)
  message("CXXOPTS_INCLUDE predefined: ${CXXOPTS_INCLUDE}")
else()
  find_path(CXXOPTS_LIBRARY NAMES cxxopts)
  if(CXXOPTS_LIBRARY)
    set(CXXOPTS_INCLUDE ${CXXOPTS_LIBRARY}/cxxopts/include)
    message("CXXOPTS found in ${CXXOPTS_INCLUDE}")
  else()
    message("CXXOPTS library not found, download into extern during make")
    ExternalProject_Add(
      cxxopts
      SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/extern/cxxopts
      GIT_REPOSITORY "https://github.com/jarro2783/cxxopts.git"
      GIT_TAG v2.2.1
      CONFIGURE_COMMAND ""
      BUILD_COMMAND ""
      INSTALL_COMMAND ""
      TEST_COMMAND ""
      GIT_SHALLOW true
    )
    set(CXXOPTS_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/extern/cxxopts/include)
  endif()
endif()

function(add_cxxopts target)
  add_dependencies(${target} cxxopts)

  target_include_directories(${target} SYSTEM PUBLIC ${CXXOPTS_INCLUDE})
endfunction()
