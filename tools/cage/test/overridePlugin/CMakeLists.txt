include(AddLLVM)

find_program(LIT_EXE lit REQUIRED)
find_program(FILECHECK_EXE NAMES
    FileCheck-${LLVM_MAJOR_VERSION}
    FileCheck
    REQUIRED
)
find_program(CLANG_COMPILER 
  NAMES clang++-${LLVM_VERSION_MAJOR} clang++
  HINTS "${LLVM_TOOLS_BINARY_DIR}"
  REQUIRED
)

set(CAGE_OVERRIDES_PLUGIN "${CMAKE_BINARY_DIR}/tools/cage/src/overridesPlugin/libcage-overrides-plugin.so")
set(CAGE_TEST_DIR "${CMAKE_CURRENT_SOURCE_DIR}/testcases")

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/lit.site.cfg.in
  ${CMAKE_CURRENT_BINARY_DIR}/lit.site.cfg
  @ONLY
)

add_lit_testsuite(
  cage-overridePlugin-tests
  "Running my-tool tests"
  ${CMAKE_CURRENT_BINARY_DIR}
  PARAMS
    site_config=${CMAKE_CURRENT_BINARY_DIR}/lit.site.cfg
  DEPENDS
    cage-overrides-plugin
)
