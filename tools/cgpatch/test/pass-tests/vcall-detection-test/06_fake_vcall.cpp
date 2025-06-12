// RUN: %cgpatchcxx --verbose clang++ %s %cppargs -emit-llvm -S | %filecheck %s

#include <iostream>

struct A {
  virtual void foo() { std::cout << "A::foo()" << std::endl; }
};

void indirect_call(void (**vtable)(), A* obj) {
  vtable[0]();  // Indirect call that resembles a vtable dispatch
}

int main() {
  A a;
  void (**fake_vtable)() = reinterpret_cast<void (**)()>(&a);  // Mimic vtable lookup

  // CHECK-NOT: Virtual call identified
  // CHECK: Call is other indirect call
  indirect_call(fake_vtable, &a);

  return 0;
}
