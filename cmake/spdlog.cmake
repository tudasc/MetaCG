include(FetchContent)

# We require spdlog to be built as a static library
set(SPDLOG_BUILD_SHARED
    OFF
    CACHE BOOL "MetaCG requires statically built spdlog to avoid lifetime issues"
)

# Use the spdlog install option to install when MetaCG is installed
set(SPDLOG_INSTALL
    ON
    CACHE INTERNAL ""
)

FetchContent_Declare(spdlog URL https://github.com/gabime/spdlog/archive/refs/tags/v1.8.2.tar.gz)
FetchContent_MakeAvailable(spdlog)

if(DEFINED spdlog_SOURCE_DIR)
  set_target_properties(spdlog PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()

function(add_spdlog_libraries target)
  target_link_libraries(${target} PUBLIC spdlog::spdlog)
endfunction()
