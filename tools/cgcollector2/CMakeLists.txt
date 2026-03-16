set(PROJECT_NAME CGCollector2)
set(TARGETS_EXPORT_NAME ${PROJECT_NAME}-target)
# Compile flags
function(default_compile_options target)
  cmake_parse_arguments(
    ARG
    ""
    ""
    "PRIVATE_FLAGS;PUBLIC_FLAGS"
    ${ARGN}
  )

  target_compile_options(
    ${target}
    PRIVATE -Wall
            -Wextra
            -pedantic
            -Wunreachable-code
            -Wwrite-strings
            -Wpointer-arith
            -Wcast-align
            -Wcast-qual
            -fno-rtti
            -g
  )

  if(ARG_PRIVATE_FLAGS)
    target_compile_options(${target} PRIVATE "${ARG_PRIVATE_FLAGS}")
  endif()

  if(ARG_PUBLIC_FLAGS)
    target_compile_options(${target} PUBLIC "${ARG_PUBLIC_FLAGS}")
  endif()
endfunction()

add_executable(
  cgcollector2
  src/CallGraphCollectionAction.cpp
  src/CallGraphNodeGenerator.cpp
  src/CGCollector2.cpp
  src/MetaDataFunctions.cpp
  src/SharedDefs.cpp
)
default_compile_options(cgcollector2)

# set log level using -DLOG_LEVEL="INFO"|"DEBUG", defaults to "INFO"
set(LOG_LEVEL
    "INFO"
    CACHE STRING ""
)
message(STATUS LOG_LEVEL=${LOG_LEVEL})
string(TOUPPER ${LOG_LEVEL} LOG_LEVEL_LOWER)
target_compile_definitions(
  cgcollector2
  PRIVATE COMPILETIME_LOG_LEVEL=SPDLOG_LEVEL_${LOG_LEVEL}
          SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_${LOG_LEVEL}
          RUNTIME_LOG_LEVEL=spdlog::level::${LOG_LEVEL_LOWER}
)

add_clang(cgcollector2)
target_include_directories(cgcollector2 PUBLIC include)
add_metacg(cgcollector2)
add_spdlog_libraries(cgcollector2)
add_json(cgcollector2)

install(
  TARGETS cgcollector2
  EXPORT ${TARGETS_EXPORT_NAME}
  RUNTIME DESTINATION bin
)

add_library(cgcollector2-plugin-interface INTERFACE)

target_sources(cgcollector2-plugin-interface PUBLIC
        FILE_SET HEADERS
        BASE_DIRS include
        FILES include/cgcollector2/interface/Plugin.h
)

target_link_libraries(cgcollector2-plugin-interface INTERFACE metacg::metacg)
add_llvm(cgcollector2-plugin-interface INTERFACE)

install(TARGETS cgcollector2-plugin-interface EXPORT cgcollector2-plugin-interfaceExport
        FILE_SET HEADERS DESTINATION include/)
