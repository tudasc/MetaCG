project(CGPatch)

# LLVM
if(NOT LLVM_ENABLE_RTTI)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

add_library(cgpatch-call-analysis STATIC src/CallAnalysis.cpp)
target_link_libraries(cgpatch-call-analysis PUBLIC ${llvm_libs})
target_include_directories(cgpatch-call-analysis PRIVATE include ${LLVM_INCLUDE_DIRS})

# Create Libraries
add_library(cgpatch-inst-pass SHARED src/CGPatchInstPass.cpp)

target_link_libraries(cgpatch-inst-pass PRIVATE cgpatch-call-analysis ${llvm_libs})
target_include_directories(cgpatch-inst-pass PRIVATE include ${LLVM_INCLUDE_DIRS})

add_metacg(cgpatch-inst-pass)
add_spdlog_libraries(cgpatch-inst-pass)
add_json(cgpatch-inst-pass)

install(TARGETS cgpatch-inst-pass LIBRARY DESTINATION lib)

add_subdirectory(wrapper)
add_subdirectory(test)
