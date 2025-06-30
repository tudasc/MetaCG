// RUN: %cgpatchcxx --points-to-analysis --patch-file /dev/null  --verbose clang++ %s -o %s.o | %filecheck %s
// XFAIL: *
// Expected to fail, because heap loads/stores are not correctly tracked by the PTA.

#include <vector>

typedef void (*FuncPtr)();
void foo() {}
void bar() {}

void callIndirect(FuncPtr fptr) {
  fptr();  // Indirect call
  // CHECK: Indirect call:
  // CHECK: may target: _Z3foov
}

int main() {
  FuncPtr* ptrs = new FuncPtr[4];
  ptrs[0] = foo;
  callIndirect(ptrs[0]);
  return 0;
}
