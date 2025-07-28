project(CGPatch)

include(GNUInstallDirs)

# LLVM
option(
  USE_FNO_RTTI
  "Disable RTTI if LLVM doesn't use it"
  ON
)

add_library(cgpatch-call-analysis STATIC src/CallAnalysis.cpp)
target_link_libraries(cgpatch-call-analysis PUBLIC ${llvm_libs})
target_include_directories(cgpatch-call-analysis PRIVATE include ${LLVM_INCLUDE_DIRS})

# Create Libraries
add_library(cgpatch-inst-pass SHARED src/CGPatchInstPass.cpp)
add_library(cgpatch-runtime STATIC src/CGPatchRuntime.cpp src/SymbolRetriever/SymbolRetriever.cpp)

target_link_libraries(cgpatch-inst-pass PRIVATE cgpatch-call-analysis ${llvm_libs})
target_include_directories(cgpatch-inst-pass PRIVATE include ${LLVM_INCLUDE_DIRS})
target_include_directories(cgpatch-inst-pass PRIVATE include)
target_include_directories(cgpatch-runtime PUBLIC include/SymbolRetriever/)

# Apply compile flags per target
foreach(tgt cgpatch-call-analysis cgpatch-inst-pass)
  target_compile_options(${tgt} PRIVATE -fPIC)
  if(NOT LLVM_ENABLE_RTTI)
    target_compile_options(${tgt} PRIVATE -fno-rtti)
  endif()
endforeach()

if(CGPATCH_USE_MPI)
  target_compile_definitions(cgpatch-runtime PRIVATE USE_MPI=1)
endif()

add_metacg(cgpatch-runtime)
add_spdlog_libraries(cgpatch-runtime)
add_json(cgpatch-runtime)

add_metacg(cgpatch-inst-pass)

install(TARGETS cgpatch-inst-pass cgpatch-runtime LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

add_subdirectory(wrapper)
add_subdirectory(test)
