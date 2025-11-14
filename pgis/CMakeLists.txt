set(PROJECT_NAME PGIS)
set(TARGETS_EXPORT_NAME ${PROJECT_NAME}-target)

add_subdirectory(lib)
add_subdirectory(tool)

if(METACG_BUILD_UNIT_TESTS)
  enable_testing()
  add_subdirectory(test/unit)
endif()
