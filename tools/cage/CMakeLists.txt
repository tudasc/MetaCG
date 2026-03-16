add_subdirectory(src)
add_subdirectory(test)

add_library(cage-plugin-interface INTERFACE)

target_sources(cage-plugin-interface PUBLIC
        FILE_SET HEADERS
        BASE_DIRS include
        FILES include/cage/interface/CaGePlugin.h
)

target_link_libraries(cage-plugin-interface INTERFACE metacg::metacg)
add_llvm(cage-plugin-interface INTERFACE)


install(TARGETS cage-plugin-interface EXPORT cage-plugin-interfaceExport
        FILE_SET HEADERS DESTINATION include/)

install(TARGETS cage cage-plugin LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
