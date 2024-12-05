# LLVM related stuff (XXX Do we actually need LLVM?)
find_package(
  LLVM
  REQUIRED
  CONFIG
)

# Clang related stuff
find_package(
  Clang
  REQUIRED
  CONFIG
)

# Test if the LLVM major version is one of the allowed ones
function(test_llvm_major_version version_string)
  # List of all supported major LLVM versions
  set(supported_llvm_versions
      10
      13
      14
      15
      16
      17
      18
  )
  # Set to indicate after loop if supported version was found
  set(valid_version FALSE)
  if(${version_string}
     IN_LIST
     supported_llvm_versions
  )
    message(STATUS "Supported LLVM Found! Version ${version_string}")
    set(valid_version TRUE)
  endif()

  if(NOT ${valid_version})
    message(SEND_ERROR "LLVM/Clang version 10, 13, 14, 15, 16, 17, and 18 are supported and tested")
  endif()
endfunction()

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
message(STATUS "Using ClangConfig.cmake in: ${Clang_DIR}")

# Check for LLVM compatibility
test_llvm_major_version(${LLVM_VERSION_MAJOR})

function(add_clang target)
  # clang specified as system lib to suppress all warnings from clang headers
  target_include_directories(${target} SYSTEM PUBLIC ${CLANG_INCLUDE_DIRS})

  target_link_libraries(
    ${target}
    PUBLIC clangTooling
           clangFrontend
           clangSerialization
           clangAST
           clangBasic
           clangIndex
  )
  if(LLVM_LINK_LLVM_DYLIB)
    target_link_libraries(${target} PUBLIC LLVM)
  else()
    llvm_map_components_to_libnames(llvm_libs support)
    target_link_libraries(${target} PUBLIC ${llvm_libs})
  endif()

endfunction()
