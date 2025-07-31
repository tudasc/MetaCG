// RUN: %cgpatchcxx clang++ %s -emit-llvm -S -o - | %filecheck %s

template <typename F>
void foo(F& f) {
  f();
}

template <typename F>
void foo_with_arg(F& f, int x) {
  f(x);
}

void bar() {}

void bar_with_arg(int x) {}

void caller(int x) {
  foo(bar);
  foo_with_arg(bar_with_arg, x);
}

// CHECK: define linkonce_odr dso_local void @_Z3fooIFvvEEvRT_(
// CHECK: call void @__metacg_indirect_call

// CHECK: define linkonce_odr dso_local void @_Z12foo_with_argIFviEEvRT_i(
// CHECK: call void @__metacg_indirect_call

// CHECK: declare void @__metacg_indirect_call(
