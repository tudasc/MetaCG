// RUN:  %cgpatchcxx clang++ %s -emit-llvm -S -o - | %filecheck %s

void foo(void) {}

void bar(void) {}

void baz(void) {}

void test_function_pointer(void) {
  void (*func_ptr1)(void) = foo;
  func_ptr1();  // Indirect call to foo

  {
    void (*func_ptr2)(void) = bar;
    func_ptr2();  // Indirect call to bar
  }

  if (1) {
    void (*func_ptr3)(void) = baz;
    func_ptr3();  // Indirect call to baz
  }
}

// CHECK: define dso_local void @_Z21test_function_pointerv
// CHECK: call void @__metacg_indirect_call(
// CHECK: call void @__metacg_indirect_call(
