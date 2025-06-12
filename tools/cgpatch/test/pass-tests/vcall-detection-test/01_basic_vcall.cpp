// RUN: %cgpatchcxx --verbose clang++ %s %cppargs -emit-llvm -S | %filecheck %s

#include <iostream>

struct A {
  virtual void foo() { std::cout << "A::foo()" << std::endl; }
};

struct B : public A {
  void foo() override { std::cout << "B::foo()" << std::endl; }
};

int main() {
  // CHECK: Traversing function: main
  A* obj = new B();
  // CHECK: Virtual call identified
  obj->foo();
  // CHECK-NOT: Virtual call identified
  // CHECK: Call is direct call
  obj->A::foo();
  delete obj;
  return 0;
}
