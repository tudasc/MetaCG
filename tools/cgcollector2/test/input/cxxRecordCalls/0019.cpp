/**
 * File: 0019.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

// The parameters in these function types have local storage, but are not inside a function.
// Inferring their destructor calls must not crash, and no edges are drawn for them.
struct S {
  ~S();
};

void (*fp)(S s) = nullptr;
typedef void F1(S s);
using F2 = void(S s);

int main() {
  S s;
  return 0;
}
