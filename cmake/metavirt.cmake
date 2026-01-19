set(METAVIRT_LOG_LEVEL
    "0"
    CACHE STRING "MetaVirt log level from 0 (least verbose) to 4 (most verbose)"
)

FetchContent_Declare(
  metavirt
  GIT_REPOSITORY https://github.com/ahueck/llvm-metavirt.git
  GIT_TAG devel
  GIT_SHALLOW 1
  FIND_PACKAGE_ARGS
)

FetchContent_MakeAvailable(metavirt)
