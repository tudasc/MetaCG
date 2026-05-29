find_program(
  LLVM_CONFIG_EXECUTABLE
  llvm-config
  REQUIRED
)

execute_process(
  COMMAND ${LLVM_CONFIG_EXECUTABLE} --prefix
  OUTPUT_VARIABLE LLVM_PREFIX
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Add the MLIR and Flang CMake modules to the search path. Otherwise, CMake won't find them.
list(
  APPEND
  CMAKE_PREFIX_PATH
  "${LLVM_PREFIX}/lib/cmake/mlir"
  "${LLVM_PREFIX}/lib/cmake/flang"
)

find_package(
  MLIR
  REQUIRED
  CONFIG
)

find_package(
  Flang
  REQUIRED
  CONFIG
)

# Test if the LLVM major version is one of the allowed ones
function(test_llvm_major_version version_string)
  # List of all supported major LLVM versions
  set(supported_llvm_versions
      18
      19
      20
      21
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
    message(WARNING "Support for LLVM Version ${version_string} is not tested! Proceed with care.")
    message(WARNING "LLVM/MLIR/Flang version 18 19 20 21 are supported and tested")
  endif()
endfunction()

message(STATUS "Found FlangConfig.cmake in: ${Flang_DIR}")
message(STATUS "Using Flang version: ${Flang_VERSION}")
message(STATUS "Found MLIRConfig.cmake in: ${MLIR_DIR}")
message(STATUS "Using MLIR version: ${MLIR_VERSION}")

test_llvm_major_version(${LLVM_VERSION_MAJOR})

function(add_flang target)
  # Before LLVM 21, this is set through CMake's TestBigEndian (deperated in CMake 3.20). As a result this doesn't seem
  # to work anymore. We now set it manually the new way. This must to be set or the compilation will fail. Note: This
  # check is not required with LLVM 21.
  if(CMAKE_CXX_BYTE_ORDER
     STREQUAL
     "BIG_ENDIAN"
  )
    target_compile_definitions(${target} PRIVATE FLANG_BIG_ENDIAN)
  elseif(
    CMAKE_CXX_BYTE_ORDER
    STREQUAL
    "LITTLE_ENDIAN"
  )
    target_compile_definitions(${target} PRIVATE FLANG_LITTLE_ENDIAN)
  else()
    message(WARNING "Endianness could not be determined.")
  endif()

  target_include_directories(${target} SYSTEM PUBLIC ${FLANG_INCLUDE_DIRS})

  find_library(
    FLANG_FRONTEND_TOOL flangFrontendTool
    PATHS ${LLVM_LIBRARY_DIR}
    NO_DEFAULT_PATH
  )
  target_link_libraries(${target} PUBLIC ${FLANG_FRONTEND_TOOL})
endfunction()
