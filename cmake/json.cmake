include(FetchContent)

if(METACG_USE_EXTERNAL_JSON)
  message("Using externally found json library")
  # Taken from https://cmake.org/cmake/help/v3.16/command/find_package.html#version-selection Should enable to use the
  # highest available version number, should the package provide sorting
  set(CMAKE_FIND_PACKAGE_SORT_ORDER NATURAL)
  set(CMAKE_FIND_PACKAGE_SORT_DIRECTION DEC)
  find_package(
    nlohmann_json
    3.10
    REQUIRED
  )
else()
  message("Using fetched release version of json library")

  # Use the json library install option to install when MetaCG is installed
  set(JSON_Install
      ON
      CACHE INTERNAL ""
  )

  FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz)
  FetchContent_MakeAvailable(json)
endif()

function(add_json target)
  target_link_libraries(${target} PRIVATE nlohmann_json::nlohmann_json)
endfunction()
