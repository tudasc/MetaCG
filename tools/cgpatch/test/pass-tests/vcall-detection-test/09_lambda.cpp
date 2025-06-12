// RUN: %cgpatchcxx --verbose clang++ %s %cppargs -emit-llvm -S | %filecheck %s

#include <iostream>

int main() {
  auto lambda = []() { std::cout << "Lambda called" << std::endl; };

  void (*funcPtr)() = lambda;

  // CHECK: Traversing function: main
  // CHECK-NOT: Virtual call identified
  // CHECK: Call is other indirect call
  funcPtr();
  return 0;
}
