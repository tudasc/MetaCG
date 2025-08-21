if(PROJECT_IS_TOP_LEVEL)
  set(CMAKE_INSTALL_INCLUDEDIR
      "include/shared-${PROJECT_VERSION}"
      CACHE PATH ""
  )
endif()

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

# find_package(<package>) call for consumers to find this project
set(package metacg)

# We put the export file there. We currently don't really use the export file (and I'm unsure if we should)
install(
  DIRECTORY include/ "${PROJECT_BINARY_DIR}/graph/export/"
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
  COMPONENT metacg_Development
  FILES_MATCHING
  PATTERN "*.h"
)

# Installation rule for the metacg library. Honestly, not sure what all this does. Following the example here:
# https://github.com/friendlyanon/cmake-init-shared-static/blob/master/cmake/install-rules.cmake
install(
  TARGETS metacg
  EXPORT metacgTargets
  RUNTIME #
          COMPONENT metacg_Runtime
  LIBRARY #
          COMPONENT metacg_Runtime NAMELINK_COMPONENT metacg_Development
  ARCHIVE #
          COMPONENT metacg_Development
  INCLUDES #
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

# Call helper function to "generate" a metacgConfigVersion file
write_basic_package_version_file(
  "${PROJECT_BINARY_DIR}/${package}ConfigVersion.cmake"
  VERSION ${PACKAGE_VERSION}
  COMPATIBILITY SameMajorVersion
)

# Allow package maintainers to freely override the path for the configs
set(metacg_INSTALL_CMAKEDIR
    "${CMAKE_INSTALL_LIBDIR}/cmake/${package}"
    CACHE PATH "CMake package config location relative to the install prefix"
)
mark_as_advanced(metacg_INSTALL_CMAKEDIR)

configure_file(
  ../cmake/install-config.cmake.in
  ${CMAKE_BINARY_DIR}/install-config.cmake
  @ONLY
)

# Install the install-config file as metacgConfig.cmake file
install(
  FILES ${CMAKE_BINARY_DIR}/install-config.cmake
  DESTINATION "${metacg_INSTALL_CMAKEDIR}"
  RENAME "${package}Config.cmake"
  COMPONENT metacg_Development
)

# Install the package config version file
install(
  FILES "${PROJECT_BINARY_DIR}/${package}ConfigVersion.cmake"
  DESTINATION "${metacg_INSTALL_CMAKEDIR}"
  COMPONENT metacg_Development
)

# Install config.h
install(
  FILES "${PROJECT_BINARY_DIR}/config.h"
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
  COMPONENT metacg_Development
)

# Install the MetaCG graph library
install(
  EXPORT metacgTargets
  NAMESPACE metacg::
  DESTINATION "${metacg_INSTALL_CMAKEDIR}"
  COMPONENT metacg_Development
)

# Install public graph library dependencies
install(TARGETS spdlog EXPORT metacgTargets)

if(NOT METACG_USE_EXTERNAL_JSON)
  install(TARGETS nlohmann_json EXPORT metacgTargets)
endif()

# Install the generated CustomMD.h header
install(
  FILES ${PROJECT_BINARY_DIR}/graph/include/metadata/CustomMD.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/metadata
  COMPONENT metacg_Development
)

if(PROJECT_IS_TOP_LEVEL)
  include(CPack)
endif()
