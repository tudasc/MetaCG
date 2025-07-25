set(CONFIG_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}/config.sh")

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/config.sh.in
  ${CONFIG_OUTPUT_PATH}
  @ONLY
)
