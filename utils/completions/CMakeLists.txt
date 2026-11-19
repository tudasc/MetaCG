add_library(
  MetaCGCompletion
  src/Completion.cpp
  src/CompletionCxxopts.cpp
  src/CompletionList.cpp
  src/CompletionLLVM.cpp
)

add_cxxopts(MetaCGCompletion)
add_llvm(MetaCGCompletion PUBLIC)

target_include_directories(MetaCGCompletion PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include/)
