add_library(MetaCGCompletion
    src/Completion.cpp
    src/CompletionCxxopts.cpp
    src/CompletionLLVM.cpp
)

add_cxxopts(MetaCGCompletion)
target_include_directories(MetaCGCompletion PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include/
    ${LLVM_INCLUDE_DIRS}
)

