/**
* File: UnknownFunctionParams.cpp
* License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
* https://github.com/tudasc/metacg/LICENSE.txt
*/

typedef void (*FuncT)();

void libraryFunction(FuncT f, int param1, int param2);

void foo() {
  // passed to lib
}

int main() {
  libraryFunction(foo, 5, 2);
  return 0;
}