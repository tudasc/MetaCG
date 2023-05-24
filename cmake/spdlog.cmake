include(FetchContent)

if(METACG_USE_EXTERNAL_SPDLOG)
  message("Using externally found spdlog library")
  find_package(
    spdlog
    1.8.2
    REQUIRED
  )
else()
  message("Using fetched release version of spdlog library")

  # We require spdlog to be built with shared library support
  set(SPDLOG_BUILD_SHARED
      ON
      CACHE INTERNAL ""
  )
  # Use the spdlog install option to install when MetaCG is installed
  set(SPDLOG_INSTALL
      ON
      CACHE INTERNAL ""
  )
  FetchContent_Declare(spdlog URL https://github.com/gabime/spdlog/archive/refs/tags/v1.8.2.tar.gz)
  FetchContent_MakeAvailable(spdlog)
endif()

function(add_spdlog_libraries target)
  target_link_libraries(${target} PRIVATE spdlog::spdlog)
endfunction()
