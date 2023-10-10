include(FetchContent)

if(METACG_USE_EXTERNAL_CXXOPTS)
  message("Using externally found cxxopts library")
  find_package(
    cxxopts
    2.2.1
    REQUIRED
  )
else()
  message("Using fetched release version of cxxopts library")

  FetchContent_Declare(
    cxxopts
    GIT_REPOSITORY https://github.com/jarro2783/cxxopts.git
    GIT_TAG v2.2.1
  )
  FetchContent_MakeAvailable(cxxopts)
endif()

function(add_cxxopts target)
  target_link_libraries(${target} PRIVATE cxxopts)
endfunction()
