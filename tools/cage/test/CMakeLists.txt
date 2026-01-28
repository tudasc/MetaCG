set(METACG_CONFIG "${CMAKE_BINARY_DIR}/utils/config/metacg-build-config")
set(TEST_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

configure_file(
  runCaGeTests.sh.in
  ${CMAKE_CURRENT_BINARY_DIR}/runCaGeTests.sh
  @ONLY
)
