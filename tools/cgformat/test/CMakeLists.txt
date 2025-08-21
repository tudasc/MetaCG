add_executable(cgformat_tester CGTester.cpp)
add_json(cgformat_tester)

macro(
  make_executable
  input
  output
)
  # Executable permission: In CMAKE 3.19 we can simply use file(CHMOD) instead of file(copy) workaround
  configure_file(
    ${input}
    ${CMAKE_CURRENT_BINARY_DIR}/tmp/${output}
    @ONLY
  )
  file(
    COPY ${CMAKE_CURRENT_BINARY_DIR}/tmp/${output}
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
    FILE_PERMISSIONS
      OWNER_READ
      OWNER_WRITE
      OWNER_EXECUTE
      GROUP_READ
      GROUP_EXECUTE
      WORLD_READ
      WORLD_EXECUTE
  )
  file(REMOVE_RECURSE ${CMAKE_CURRENT_BINARY_DIR}/tmp)
endmacro()

function(
  configure_script
  input
  output
)
  set(CGFORMAT_TEST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/input)
  set(CGFORMAT_EXE ${CMAKE_CURRENT_BINARY_DIR}/../cgformat)
  set(CGFORMAT_TESTER_EXE ${CMAKE_CURRENT_BINARY_DIR}/cgformat_tester)

  # To get execute permission: create run.sh in the binary dir, and copy it to scripts folder with permission
  make_executable(${input} ${output})
endfunction()

configure_script(run_cgformat_tests.sh.in run_cgformat_tests.sh)
