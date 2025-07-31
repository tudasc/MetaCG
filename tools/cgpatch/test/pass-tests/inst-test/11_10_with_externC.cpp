// RUN:  %cgpatchcxx clang++ %s -emit-llvm -S -o - | %filecheck %s

#include <stdio.h>
// Functions to be used as function pointers
// Functions to be used as function pointers
void foo(void) { printf("foo\n"); }

void bar(void) { printf("bar\n"); }

// Function returning a function pointer to a function taking void and returning void
void (*get_function(int id))(void) {
  if (id == 1)
    return foo;
  return bar;
}

// Function that returns a function pointer, adjusted to match the type
void (*get_function2(void (*func_ptr)(void)))(void) {
  return func_ptr;  // Returns the function pointer itself
}

// Test function to demonstrate complex function pointer usage
void test_return_function_pointer(void) {
  // Function pointer that returns a function pointer
  void (*(*func_ptr)(void (*)(void)))(void) = get_function2;

  // Retrieve a function pointer based on the ID
  void (*func_ptr2)(void) = func_ptr(get_function(1));
  func_ptr2();  // Indirect call to foo

  func_ptr2 = func_ptr(get_function(0));
  func_ptr2();  // Indirect call to bar
}

int main() {
  test_return_function_pointer();
  return 0;
}
// CHECK: define dso_local void @_Z28test_return_function_pointerv()
// CHECK: call void @__metacg_indirect_call(
// CHECK: call void @__metacg_indirect_call(
// CHECK: declare void @__metacg
