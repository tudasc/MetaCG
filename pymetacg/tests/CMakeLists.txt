include(spdlog)
include(json)

enable_testing()

find_package(
  Pytest
  8.2.1
  REQUIRED
)

add_library(dummy_md SHARED DummyMD.cpp)
add_spdlog_libraries(dummy_md)
add_json(dummy_md)
target_link_libraries(dummy_md PRIVATE metacg::metacg)

pytest_discover_tests(
    pymetacg_tests
    PYTHON_PATH_PREPEND
        $<TARGET_FILE_DIR:pymetacg>
    ENVIRONMENT LD_PRELOAD=$<TARGET_FILE:dummy_md>
    TRIM_FROM_NAME "^test_"
    DEPENDS pymetacg dummy_md
)
