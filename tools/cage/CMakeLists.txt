add_subdirectory(src)
add_subdirectory(test)

install(TARGETS cage cage-plugin LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
