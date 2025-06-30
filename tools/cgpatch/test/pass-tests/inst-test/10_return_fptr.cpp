// RUN:  %cgpatchcxx clang++ %s -emit-llvm -S -o - | %filecheck %s

#include <stdio.h>
void foo(void) { printf("foo\n"); }

void bar(void) { printf("bar\n"); }
void (*get_function(int id))(void) {
  if (id == 1)
    return foo;
  return bar;
}

void test_return_function_pointer(void) {
  void (*func_ptr)(void) = get_function(1);
  func_ptr();  // Indirect call to foo

  func_ptr = get_function(0);
  func_ptr();  // Indirect call to bar
}

// CHECK: define dso_local void @_Z28test_return_function_pointerv()
// CHECK: call void @__metacg_indirect_call(
// CHECK: call void @__metacg_indirect_call(
// CHECK: declare void @__metacg_indirect_call(
