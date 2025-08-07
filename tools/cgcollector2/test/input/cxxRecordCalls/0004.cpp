/**
* File: GlobalHeapClassDirectCall.cpp
* License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
* https://github.com/tudasc/metacg/LICENSE.txt
*/

class localClass {
 public:
  int operator()(int a, int b, int c) { return a + b * c; }
};

int main() {
  auto c = (new localClass())->operator()(1, 2, 3);
  return c;
}