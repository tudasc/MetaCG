// RUN:  %cgpatchcxx clang++ %s -emit-llvm -S -o - | %filecheck %s

#include <stdio.h>
#include <stdlib.h>

void foo(void) { printf("foo\n"); }

void bar(void) { printf("bar\n"); }

void test_dynamic_function_pointers(void) {
  void (**func_ptr_arr)(void) = (void (**)(void))malloc(2 * sizeof(void (*)(void)));
  if (func_ptr_arr == NULL) {
    perror("malloc failed");
    exit(EXIT_FAILURE);
  }
  func_ptr_arr[0] = foo;
  func_ptr_arr[1] = bar;

  func_ptr_arr[0]();  // Indirect call to foo
  func_ptr_arr[1]();  // Indirect call to bar

  free(func_ptr_arr);
}

// CHECK: define dso_local void @_Z30test_dynamic_function_pointersv()
// CHECK: call void @__metacg_indirect_call(
// CHECK: call void @__metacg_indirect_call(
// CHECK: declare void @__metacg_indirect_call(
