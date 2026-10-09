// RUN: %clang -Xclang -load -Xclang %plugin -Xclang -add-plugin -Xclang cage-overrides-plugin %s -S -emit-llvm -o - | %FileCheck %s

struct A {
  virtual void foo() const {}
};

struct B : A {
  void foo() const override {}
};

int main() {
    A* a = new B();
    a->foo();
    return 0;
}

// CHECK-DAG: @[[A_ANNOT:[^ ]+]] = private unnamed_addr constant [13 x i8] c"_ZNK1A3fooEv\00"
// CHECK-DAG: @[[PAYLOAD:[^ ]+]] = private unnamed_addr constant [15 x i8] c"overridePlugin\00"
// CHECK-DAG: @[[ARGS:[^ ]+]] = private unnamed_addr constant { [15 x i8] } { [15 x i8] ptrtoint (ptr @[[PAYLOAD]] to [15 x i8]) }, section "llvm.metadata"
// CHECK: @llvm.global.annotations = appending global
// CHECK-SAME: { ptr @_ZNK1B3fooEv, ptr @[[A_ANNOT]], ptr
// CHECK-SAME: ptr @[[ARGS]] }