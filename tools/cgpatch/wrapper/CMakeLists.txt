project(wrapper)

# Configure wrapper script

# make_executable: Copy the input file to the output file and make it executable Source:
# https://git.rwth-aachen.de/tuda-sc/projects/capi/-/blob/devel/scripts/CMakeLists.txt?ref_type=heads
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
  set(METACG_INSTALL_DIR ${CMAKE_INSTALL_PREFIX})

  make_executable(${input} ${output})
endfunction()

configure_script(patchcc.in patchcc)
configure_script(patchcxx.in patchcxx)

install(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/patchcc" "${CMAKE_CURRENT_BINARY_DIR}/patchcxx"
  DESTINATION bin
  PERMISSIONS
    OWNER_READ
    OWNER_WRITE
    OWNER_EXECUTE
    GROUP_READ
    GROUP_EXECUTE
    WORLD_READ
    WORLD_EXECUTE
)
