find ./pgis/lib -iname "*.cpp" -exec clang-format -i {} \;
find ./pgis/lib -iname "*.h" -exec clang-format -i {} \;
find ./pgis/tool -iname "*.cpp" -exec clang-format -i {} \;
find ./pgis/tool -iname "*.h" -exec clang-format -i {} \;
find ./pgis/test -iname "*.cpp" -exec clang-format -i {} \;
find ./graph -iname ".h" -exec clang-format -i {} \;
find ./cgcollector/lib -iname ".cpp" -exec clang-format -i {} \;
find ./cgcollector/lib -iname ".h" -exec clang-format -i {} \;
find ./cgcollector/tools -iname ".cpp" -exec clang-format -i {} \;
find ./cgcollector/tools -iname ".h" -exec clang-format -i {} \;

# Apply CMake formatting
find . -name "CMakeLists.txt" -exec cmake-format -i {} \;
find ./cmake -name "*.cmake" -type f -exec cmake-format -i {} \;