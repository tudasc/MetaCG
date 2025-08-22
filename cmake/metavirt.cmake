add_compile_definitions(VIRTCALL_LOG_LEVEL)
FetchContent_Declare(
  metavirt
  GIT_REPOSITORY https://github.com/ahueck/llvm-metavirt.git
  GIT_TAG devel
  GIT_SHALLOW 1
  FIND_PACKAGE_ARGS
)

FetchContent_MakeAvailable(metavirt)
