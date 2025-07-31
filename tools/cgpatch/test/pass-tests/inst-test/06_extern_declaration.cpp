// RUN: %split_file %s %t
// RUN: %cgpatchcxx clang++ -emit-llvm -S -o - %t/test_function_pointer.cpp | %filecheck %s

//--- test_function_pointer.cpp

extern void foo(void);  // Forward declaration of foo
extern void bar(void);  // Forward declaration of bar
void test_function_pointer(void) {
  void (*func_ptr)(void);

  func_ptr = foo;
  func_ptr();

  func_ptr = bar;
  func_ptr();
}

//--- impl_functions.cpp
// Implementation of the functions that `func_ptr` points to
void foo(void) {
  // Implementation of foo
}

void bar(void) {
  // Implementation of bar
}

// CHECK: define dso_local void @_Z21test_function_pointerv(
// CHECK: call void @__metacg_indirect_call(
// CHECK: call void @__metacg_indirect_call(
// CHECK: declare void @__metacg_indirect_call(
