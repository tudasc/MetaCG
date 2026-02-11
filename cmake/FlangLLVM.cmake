find_package(
  Clang
  REQUIRED
  CONFIG
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

message(STATUS "Found FlangConfig.cmake in: ${Flang_DIR}")
message(STATUS "Using Flang version: ${Flang_VERSION}")
message(STATUS "Found MLIRConfig.cmake in: ${MLIR_DIR}")
message(STATUS "Using MLIR version: ${MLIR_VERSION}")

function(add_flang target)
  target_compile_definitions(${target} PRIVATE FLANG_LITTLE_ENDIAN)

  target_include_directories(${target} SYSTEM PUBLIC ${FLANG_INCLUDE_DIRS})

  find_library(
    FLANG_FRONTEND_TOOL flangFrontendTool
    PATHS ${LLVM_LIBRARY_DIR}
    NO_DEFAULT_PATH
  )
  target_link_libraries(${target} PUBLIC ${FLANG_FRONTEND_TOOL})
endfunction()
