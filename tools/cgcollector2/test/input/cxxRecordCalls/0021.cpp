/**
 * File: 0021.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

// The backing array of a std::initializer_list of class type is a temporary of array type, which has no
// CXXRecordDecl. Inferring its destructor call must not crash, and the edge goes to the element destructor.
// A minimal std::initializer_list is declared here so the test does not depend on the standard library headers.
namespace std {
using size_t = decltype(sizeof(0));
template <class E>
class initializer_list {
  const E* b;
  size_t n;
};
}  // namespace std

struct S {
  S(int);
  ~S();
};

void take(std::initializer_list<S>);

int main() {
  take({1, 2});
  return 0;
}
