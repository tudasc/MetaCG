// RUN:  %cgpatchcxx clang++ %s -emit-llvm -S -o - | %filecheck %s

#include <stdio.h>
void foo(void) { printf("foo\n"); }

void bar(void) { printf("bar\n"); }

void foobar(void) { printf("foobar\n"); }

struct FuncStruct {
  void (*func_ptr)(void);
};

int main() {
  // Array of function pointers
  void (*func_ptr_arr[2])(void) = {foo, bar};
  for (int i = 0; i < 2; ++i) {
    func_ptr_arr[i]();  // Indirect calls to foo and bar
  }

  // Function pointer in a struct
  struct FuncStruct fs;
  fs.func_ptr = foobar;
  fs.func_ptr();  // Indirect call to foo

  return 1;
}

// CHECK: define dso_local noundef i32 @main()
// CHECK: call void @__metacg_indirect_call
// CHECK: call void @__metacg_indirect_call
// NOT: call void @__metacg_indirect_call
// CHECK: declare void @__metacg_indirect_call(
