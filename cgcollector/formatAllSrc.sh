#!/usr/bin/env bash


find ./lib -regex ".*/*.[cpph]" -not -type d -exec clang-format -i {} \;
find ./test -regex ".*/*.[cpph]" -not -type d -exec clang-format -i {} \;
find ./tools -regex ".*/*.[cpph]" -not -type d -exec clang-format -i {} \;
