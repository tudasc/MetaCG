enable_testing()

find_package(
  Pytest
  8.2.1
  REQUIRED
)

pytest_discover_tests(
    pymetacg_tests
    PYTHON_PATH_PREPEND
        $<TARGET_FILE_DIR:pymetacg>
    TRIM_FROM_NAME "^test_"
    DEPENDS pymetacg
)
