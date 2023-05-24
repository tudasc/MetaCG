# Apply formatting only to tracked changes
git-clang-format devel

# Apply CMake formatting
find . -name "CMakeLists.txt" -exec cmake-format -i {} \;
find ./cmake -name "*.cmake" -type f -exec cmake-format -i {} \;

