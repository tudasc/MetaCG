// RUN: %clang -Xclang -load -Xclang %plugin -Xclang -add-plugin -Xclang cage-overrides-plugin %s -S -emit-llvm -o - | %FileCheck %s

struct A {
  virtual void foo() {}
  virtual void bar() {}
};

struct B : A {
  void foo() override {}
  void bar() override {}
};

int main() {
    A* a = new B();
    a->foo();
    a->bar();
    return 0;
}

// CHECK-DAG: @[[A_FOO_ANNOT:[^ ]+]] = private unnamed_addr constant [12 x i8] c"_ZN1A3fooEv\00"
// CHECK-DAG: @[[A_BAR_ANNOT:[^ ]+]] = private unnamed_addr constant [12 x i8] c"_ZN1A3barEv\00"
// CHECK-DAG: @[[PAYLOAD:[^ ]+]] = private unnamed_addr constant [15 x i8] c"overridePlugin\00"
// CHECK-DAG: @[[ARGS_FOO:[^ ]+]] = private unnamed_addr constant { [15 x i8] } { [15 x i8] ptrtoint (ptr @[[PAYLOAD]] to [15 x i8]) }, section "llvm.metadata"
// CHECK-DAG: @[[ARGS_BAR:[^ ]+]] = private unnamed_addr constant { [15 x i8] } { [15 x i8] ptrtoint (ptr @[[PAYLOAD]] to [15 x i8]) }, section "llvm.metadata"
// CHECK: @llvm.global.annotations = appending global
// CHECK-SAME: { ptr @_ZN1B3fooEv, ptr @[[A_FOO_ANNOT]], ptr
// CHECK-SAME: ptr @[[ARGS_FOO]] }
// CHECK-SAME: { ptr @_ZN1B3barEv, ptr @[[A_BAR_ANNOT]], ptr
// CHECK-SAME: ptr @[[ARGS_BAR]] }