// RUN: %cgpatchcxx --verbose clang++ %s %cppargs -emit-llvm -S | FileCheck %s

#include <iostream>

void foo() { std::cout << "A::foo()" << std::endl; }

extern "C" void call_ptr(void (*ptr)()) {
  // CHECK: Traversing function: call_ptr
  // CHECK-NOT: Virtual call identified
  // CHECK: Call is other indirect call
  ptr();
}

int main() {
  call_ptr(foo);
  return 0;
}
