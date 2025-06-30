// RUN:  %cgpatchcxx clang++ %s -emit-llvm -S -o - | %filecheck %s

#include <cstdio>

// Declare C-style functions with extern "C"
extern "C" void foo(void);
extern "C" void bar(void);

// Define C-style functions
extern "C" void foo(void) { printf("foo\n"); }

extern "C" void bar(void) { printf("bar\n"); }

// Function that returns a function pointer based on an ID
extern "C" void (*get_function(int id))(void) {
  if (id == 1)
    return foo;
  return bar;
}

// Template class to manage function pointers
template <int ID>
class FunctionManager {
 public:
  static void (*get_function_pointer(void))(void) { return get_function(ID); }
};

// Specialize the template for a different ID
template <>
class FunctionManager<2> {
 public:
  static void (*get_function_pointer(void))(void) {
    return get_function(1);  // Return a function pointer for ID 1
  }
};

// Function that returns a function pointer from another function
extern "C" void (*get_function_from_template(int id))(void) {
  switch (id) {
    case 1:
      return FunctionManager<1>::get_function_pointer();
    case 2:
      return FunctionManager<2>::get_function_pointer();
    default:
      return nullptr;
  }
}

// Test function demonstrating complex function pointer usage
void test_complex_function_pointer(void) {
  // Function pointer that returns another function pointer
  void (*(*func_ptr_from_template)(int))(void) = get_function_from_template;

  // Retrieve and call a function pointer based on the ID
  void (*func_ptr)(void) = func_ptr_from_template(1);
  func_ptr();  // Indirect call to foo

  func_ptr = func_ptr_from_template(2);
  func_ptr();  // Indirect call to foo (as specialized template returns foo)
}

// CHECK: define dso_local void @_Z29test_complex_function_pointerv()
// CHECK: call void @__metacg_indirect_call(
// CHECK: call void @__metacg_indirect_call(
// CHECK: call void @__metacg_indirect_call(
