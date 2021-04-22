find lib/ -iname "*.[cpp]" -exec clang-format -i {} \;
find lib/ -iname "*.[h]" -exec clang-format -i {} \;
find tool -iname "*.[cpp]" -exec clang-format -i {} \;
find tool -iname "*.[h]" -exec clang-format -i {} \;
find test -iname "*.[cpp]" -exec clang-format -i {} \;
