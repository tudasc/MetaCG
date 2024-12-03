// RUN: %cgpatchcxx --verbose clang++ %s %cppargs -emit-llvm -S | FileCheck %s

#include <iostream>

struct A {
  virtual void foo() { std::cout << "A::foo()" << std::endl; }
};

extern "C" void call_virtual(void (A::*ptr)(), A* obj) {
  // CHECK: Traversing function: call_virtual
  // CHECK-NOT: Virtual call identified
  // CHECK: Call is other indirect call
  (obj->*ptr)();
}

int main() {
  A a;
  call_virtual(&A::foo, &a);
  return 0;
}
