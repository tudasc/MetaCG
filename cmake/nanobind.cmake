include(FetchContent)

if (CMAKE_VERSION VERSION_LESS 3.18)
  set(DEV_MODULE Development)
else()
  set(DEV_MODULE Development.Module)
endif()

find_package(Python 3.8 COMPONENTS Interpreter ${DEV_MODULE} REQUIRED)

message("Found Python ${Python_VERSION}")


if (METACG_USE_EXTERNAL_NANOBIND)
  if (NOT DEFINED nanobind_ROOT)
    execute_process(
      COMMAND "${Python_EXECUTABLE}" -m nanobind --cmake_dir
      OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE nanobind_ROOT)
  endif()

  find_package(nanobind CONFIG REQUIRED)

  message("Using externally found nanobind library (${nanobind_DIR})")
else()
  message("Using fetched release version of nanobind library")

  FetchContent_Declare(
      nanobind
      GIT_REPOSITORY https://github.com/wjakob/nanobind.git
      GIT_TAG v2.5.0
  )
  FetchContent_MakeAvailable(nanobind)
endif()