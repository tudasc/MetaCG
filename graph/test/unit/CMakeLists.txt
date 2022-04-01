# Now simply link against gtest or gtest_main as needed. Eg
add_executable(
  libtests
  MCGManagerTest.cpp
  MCGReaderTest.cpp
  MCGWriterTest.cpp
  UtilTest.cpp
)

target_link_libraries(libtests gtest_main)
add_pgis(libtests)

# add_library(ipcg)
target_link_libraries(libtests mcg)

add_test(NAME libraryTests COMMAND libtests)
