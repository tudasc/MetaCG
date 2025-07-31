// RUN: %cgpatchcxx clang++ %s -emit-llvm -S -o - | %filecheck %s

int bar_with_arg(int x) { return x * x; }

class A {
 public:
  virtual int foo_with_arg(int (*f)(int x), int x) { return f(x); }
};

class B : public A {
 public:
  virtual int foo_with_arg(int (*f)(int x), int x) { return -f(x); }
};

void caller() {
  B b;

  A* a_pointer = &b;

  int x = a_pointer->foo_with_arg(bar_with_arg, 4);
}

// CHECK: define linkonce_odr dso_local noundef i32 @_ZN1B12foo_with_argEPFiiEi(
// CHECK: call void @__metacg_indirect_call(
// CHECK: call noundef i32 %

// CHECK: define linkonce_odr dso_local noundef i32 @_ZN1A12foo_with_argEPFiiEi(
// CHECK: call void @__metacg_indirect_call(
// CHECK: call noundef i32 %{{[0-9]+}}(

// CHECK: declare void @__metacg_indirect_call(
