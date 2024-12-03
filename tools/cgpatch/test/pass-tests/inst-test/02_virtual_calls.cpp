// RUN: %cgpatchcxx clang++ %s -emit-llvm -S -o - | %filecheck %s
#include <stdio.h>

class A {
 public:
  virtual int foo() {
    printf("A");
    return 1;
  }
};

class B : public A {
 public:
  int foo() {
    printf("B");
    return 2;
  }
};

void caller() {
  A a;

  B* b_pointer = new B();
  A* a_pointer = new A();

  a_pointer->foo();
  b_pointer->foo();
  a.foo();

  delete a_pointer;
  delete b_pointer;
}

// CHECK-NOT: call void @__metacg_indirect_call

// CHECK: declare void @__metacg_indirect_call(
