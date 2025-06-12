// RUN: %cgpatchcxx --verbose clang++ %s %cppargs -emit-llvm -S | %filecheck %s

#include <iostream>

struct A {
  void foo() { std::cout << "A::foo()" << std::endl; }
};

extern "C" void call_member(void (A::*ptr)(), A* obj) {
  // CHECK: Traversing function: call_member
  A a;
  // CHECK-NOT: Virtual call identified
  // CHECK: Call is other indirect call
  (obj->*ptr)();
}

int main() {
  A a;
  call_member(&A::foo, &a);
  return 0;
}
