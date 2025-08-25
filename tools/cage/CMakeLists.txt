option(
  CAGE_USE_METAVIRT
  ON
  "Rely on metavirt for virtual call handling"
)

if(${CAGE_USE_METAVIRT})
  include(metavirt)
endif()

add_subdirectory(src)

install(TARGETS cage cage-plugin LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
