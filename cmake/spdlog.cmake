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

  FetchContent_Declare(spdlog URL https://github.com/gabime/spdlog/archive/refs/tags/v1.8.2.tar.gz)
  FetchContent_MakeAvailable(spdlog)

  # Only install when spdlog is desired as automatically downloaded library
  install(
    TARGETS spdlog
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
  )
endif()

function(add_spdlog_libraries target)
  target_link_libraries(${target} spdlog::spdlog)
endfunction()
