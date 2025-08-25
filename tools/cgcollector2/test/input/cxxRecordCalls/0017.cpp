/**
* File: LocalStackClassDirectCall.cpp
* License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
* https://github.com/tudasc/metacg/LICENSE.txt
*/

int main() {
  class localClass {
   public:
    int operator()(int a, int b, int c) { return a + b * c; }
  };
  auto c = localClass()(1, 2, 3);
  return c;
}
