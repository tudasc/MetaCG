// RUN: %cgpatchcxx clang++ %s -emit-llvm -S -o - | %filecheck %s
#include <stdio.h>

int bar_with_arg(int (*fptr)(void*, int), void* context) { return fptr(context, 16); }

class A {
 public:
  virtual int foo_with_arg(int x) { return x * x; }
};

class B : public A {
 public:
  virtual int foo_with_arg(int x) { return -(x * x); }
};

int forwarder(void* context, int i0) { return static_cast<A*>(context)->foo_with_arg(i0); }

void caller() {
  B b;
  A a;
  A* pointer = &b;

  printf("%d", bar_with_arg(forwarder, pointer));
}

// CHECK: define dso_local noundef i32 @_Z12bar_with_argPFiPviES_(
// CHECK: call void @__metacg_indirect_call(
// CHECK: call noundef i32 %{{[0-9]+}}(

// CHECK: declare void @__metacg_indirect_call(
