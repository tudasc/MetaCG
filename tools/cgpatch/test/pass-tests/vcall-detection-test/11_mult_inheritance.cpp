// RUN: %cgpatchcxx --verbose clang++ %s %cppargs -emit-llvm -S | %filecheck %s

#include <iostream>

struct Base1 {
  virtual void foo() { std::cout << "Base1::foo()" << std::endl; }
};

struct Base2 {
  virtual void bar() { std::cout << "Base2::bar()" << std::endl; }
};

struct Derived : public Base1, public Base2 {
  void foo() override { std::cout << "Derived::foo()" << std::endl; }

  void bar() override { std::cout << "Derived::bar()" << std::endl; }
};

int main() {
  Base1* b1 = new Derived;
  Base2* b2 = new Derived;

  b1->foo();  // Virtual call through Base1 pointer
  b2->bar();  // Virtual call through Base2 pointer

  // CHECK: Traversing function: main
  // CHECK: Virtual call identified
  // CHECK: Virtual call identified

  delete b1;
  delete b2;
  return 0;
}
