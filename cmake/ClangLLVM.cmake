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

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
message(STATUS "Using ClangConfig.cmake in: ${Clang_DIR}")

if(NOT
   ((${LLVM_VERSION_MAJOR}
     VERSION_EQUAL
     "10")
    OR (${LLVM_VERSION_MAJOR}
        VERSION_EQUAL
        "13")
    OR (${LLVM_VERSION_MAJOR}
        VERSION_EQUAL
        "14")
    OR (${LLVM_VERSION_MAJOR}
        VERSION_EQUAL
        "15"))
)
  message(SEND_ERROR "LLVM/Clang version 10, 13, 14 and 15 are supported and tested.")
endif()

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
