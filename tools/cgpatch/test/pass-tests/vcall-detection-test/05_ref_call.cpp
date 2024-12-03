// RUN: %cgpatchcxx --verbose clang++ %s %cppargs -emit-llvm -S | FileCheck %s

#include <iostream>

struct A {
  virtual void foo() { std::cout << "A::foo()" << std::endl; }
};

struct B : public A {
  void foo() override { std::cout << "B::foo()" << std::endl; }
};

extern "C" void call_virtual(A& a) {
  // CHECK: Traversing function: call_virtual
  // CHECK: Virtual call identified
  a.foo();
}

int main() {
  B b;
  call_virtual(b);
  return 0;
}
