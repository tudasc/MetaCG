# Renamed to cgsimpletester2 to avoid clash with target from old cgcollector
add_executable(cgsimpletester2 CGSimpleTester.cpp)
add_json(cgsimpletester2)

target_link_libraries(cgsimpletester2 PUBLIC metacg::metacg)