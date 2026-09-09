// RUN: %clang -Xclang -load -Xclang %plugin -Xclang -add-plugin -Xclang cage-overrides-plugin %s -S -emit-llvm -o - | %FileCheck %s

struct A {
  virtual void foo() {}
};

int main() {
  A a;
  a.foo();
  return 0;
}

// CHECK-NOT: c"overridePlugin\00"
