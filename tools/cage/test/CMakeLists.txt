set(METACG_CONFIG "${CMAKE_BINARY_DIR}/utils/config/metacg-config")
set(TEST_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/01_simple")
configure_file(01_simple/Makefile.in
        ${CMAKE_CURRENT_BINARY_DIR}/01_simple/Makefile
        @ONLY)