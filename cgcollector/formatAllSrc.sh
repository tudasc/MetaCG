#!/usr/bin/env bash

find ./lib -iname "*.h" -not -type d -exec clang-format -i {} \;
find ./lib -iname "*.cpp" -not -type d -exec clang-format -i {} \;
find ./tools -iname "*.h" -not -type d -exec clang-format -i {} \;
find ./tools -iname "*.cpp" -not -type d -exec clang-format -i {} \;
find ./test -iname "*.h" -not -type d -exec clang-format -i {} \;
find ./test -iname "*.cpp" -not -type d -exec clang-format -i {} \;
