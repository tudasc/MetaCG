find lib/ -iname "*.[h|cpp]" -exec clang-format -i {} \;
find tool -iname "*.[h|cpp]" -exec clang-format -i {} \;
