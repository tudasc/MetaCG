include(ExternalProject)

if(DEFINED JSON_INCLUDE)
  message("JSON_INCLUDE predefined: ${JSON_INCLUDE}")
else()
  find_path(JSON_LIBRARY NAMES json)
  if(JSON_LIBRARY)
    set(JSON_INCLUDE ${JSON_LIBRARY}/json/single_include)
    message("JSON found in ${JSON_INCLUDE}")
  else()
    message("JSON library not found, download into extern during make")
    ExternalProject_Add(
      json
      SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/extern/json
      GIT_REPOSITORY "https://github.com/nlohmann/json.git"
      GIT_TAG master
      CONFIGURE_COMMAND ""
      BUILD_COMMAND ""
      INSTALL_COMMAND ""
      TEST_COMMAND ""
      GIT_SHALLOW true
    )
    set(JSON_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/extern/json/single_include)
  endif()
endif()

function(add_json target)
  add_dependencies(${target} json)

  target_include_directories(${target} SYSTEM PUBLIC ${JSON_INCLUDE})
endfunction()
