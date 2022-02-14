set(PROJECT_NAME PGIS)
set(TARGETS_EXPORT_NAME ${PROJECT_NAME}-target)

enable_testing()

add_subdirectory(lib)
add_subdirectory(tool)
add_subdirectory(test/unit)
