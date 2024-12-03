// RUN: %cgpatchcxx --verbose clang++ %s %cppargs -emit-llvm -S | FileCheck %s

// XFAIL: *

#include <iostream>

struct A {
  A() {
    // Note: Virtual functions in the constructor should be direct calls according to the standard.
    //       However, the IR emitted by Clang still loads these functions from the vtable.
    // CHECK: Traversing function: _ZN1AC2Ev
    // CHECK-NOT: Virtual call identified
    foo();
  }
  virtual void foo() { std::cout << "A::foo()" << std::endl; }
};

int main() {
  A a;
  return 0;
}
