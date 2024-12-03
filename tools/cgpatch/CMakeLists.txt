project(CGPatch)

# LLVM
if(NOT LLVM_ENABLE_RTTI)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")
endif()

option(
  CGPATCH_USE_MPI
  "On or Of"
  ON
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

add_library(cgpatch-call-analysis STATIC src/CallAnalysis.cpp)
target_link_libraries(cgpatch-call-analysis PUBLIC ${llvm_libs})
target_include_directories(cgpatch-call-analysis PRIVATE include)

# Create Libraries
add_library(cgpatch-inst-pass SHARED src/cgpatch-inst-pass.cpp)
add_library(cgpatch-runtime STATIC src/cgpatch-runtime.cpp src/SymbolRetriever/SymbolRetriever.cpp)

target_link_libraries(cgpatch-inst-pass PRIVATE cgpatch-call-analysis ${llvm_libs})
target_include_directories(cgpatch-inst-pass PRIVATE include)
target_include_directories(cgpatch-runtime PUBLIC include/SymbolRetriever/)

if(CGPATCH_USE_MPI)
  target_compile_definitions(cgpatch-runtime PRIVATE USE_MPI=1)
endif()

add_metacg(cgpatch-runtime)
add_spdlog_libraries(cgpatch-runtime)
add_json(cgpatch-runtime)

add_metacg(cgpatch-inst-pass)
add_spdlog_libraries(cgpatch-inst-pass)
add_json(cgpatch-inst-pass)

install(TARGETS cgpatch-inst-pass cgpatch-runtime LIBRARY DESTINATION lib)

add_subdirectory(wrapper)
add_subdirectory(test)
