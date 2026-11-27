add_library(
  MetaCGCompletion
  src/Completion.cpp
  src/CompletionCxxopts.cpp
  src/CompletionList.cpp
)

add_cxxopts(MetaCGCompletion)
if(COMMAND add_llvm)
  target_sources(MetaCGCompletion PRIVATE src/CompletionLLVM.cpp)
  add_llvm(MetaCGCompletion PUBLIC)
endif()

target_include_directories(MetaCGCompletion PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include/)
