/**
 * File: 0020.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

// The temporary in the initializer of a global variable is not inside a function.
// Inferring its destructor call must not crash, and no edge is drawn for it.
struct S {
  ~S();
};

int g = (S(), 0);

int main() {
  S s;
  return g;
}
