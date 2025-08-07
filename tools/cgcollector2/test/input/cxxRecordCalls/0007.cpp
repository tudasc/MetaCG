/**
* File: GlobalLambda.cpp
* License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
* https://github.com/tudasc/metacg/LICENSE.txt
*/

auto globalLambda = [](int a, int b, int c) { return a + b + c; };

int main() {
  auto c = globalLambda;
  return c(1, 2, 3);
}