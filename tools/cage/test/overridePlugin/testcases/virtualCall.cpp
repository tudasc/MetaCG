// RUN: %clang -Xclang -load -Xclang %plugin -Xclang -add-plugin -Xclang cage-overrides-plugin %s -S -emit-llvm -o - | %FileCheck %s

struct A {
  virtual void foo() {}
  virtual void bar() {}
};

struct B : A {
  void foo() override {}
};

int main() {
    A* a = new B();
    a->foo();


    return 1;
}

// CHECK-DAG: @[[BASE_ANNOT:[^ ]+]] = private unnamed_addr constant [12 x i8] c"_ZN1A3fooEv\00", section "llvm.metadata"
// CHECK-DAG: @[[PAYLOAD:[^ ]+]] = private unnamed_addr constant [15 x i8] c"overridePlugin\00"
// CHECK-DAG: @[[ARGS:[^ ]+]] = private unnamed_addr constant { [15 x i8] } { [15 x i8] ptrtoint (ptr @[[PAYLOAD]] to [15 x i8]) }, section "llvm.metadata"
// CHECK: @llvm.global.annotations = appending global
// CHECK-SAME: { ptr @_ZN1B3fooEv, ptr @[[BASE_ANNOT]], ptr
// CHECK-SAME: ptr @[[ARGS]] }
