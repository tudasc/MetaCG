// RUN: %cgpatchcxx --verbose clang++ %s %cppargs -emit-llvm -S | FileCheck %s

#include <iostream>

struct A {
  virtual ~A() {
    // CHECK: Virtual call identified
    foo();
  }
  virtual void foo() { std::cout << "A::foo()" << std::endl; }
};

struct B : public A {
  void foo() override { std::cout << "B::foo()" << std::endl; }
};

int main() {
  A* a = new B();
  delete a;
  return 0;
}
