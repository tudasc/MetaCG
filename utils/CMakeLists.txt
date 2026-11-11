add_subdirectory(config)

install(PROGRAMS extract_metacg DESTINATION bin)

if(METACG_BUILD_GRAPH_TOOLS)
  add_subdirectory(completions)
endif()
