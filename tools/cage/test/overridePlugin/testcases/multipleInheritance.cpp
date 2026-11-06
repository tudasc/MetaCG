// RUN: %clang -Xclang -load -Xclang %plugin -Xclang -add-plugin -Xclang cage-overrides-plugin %s -S -emit-llvm -o - | %FileCheck %s

struct A {
  virtual void foo() {}
};

struct B {
  virtual void bar() {}
};

struct C : A, B {
  void foo() override {}
  void bar() override {}
};

int main() {
    C* c = new C();
    c->foo();
    c->bar();
    return 0;
}

// CHECK-DAG: @[[A_ANNOT:[^ ]+]] = private unnamed_addr constant [12 x i8] c"_ZN1A3fooEv\00"
// CHECK-DAG: @[[B_ANNOT:[^ ]+]] = private unnamed_addr constant [12 x i8] c"_ZN1B3barEv\00"
// CHECK-DAG: @[[PAYLOAD:[^ ]+]] = private unnamed_addr constant [15 x i8] c"overridePlugin\00"
// CHECK-DAG: @[[ARGS_A:[^ ]+]] = private unnamed_addr constant { [15 x i8] } { [15 x i8] ptrtoint (ptr @[[PAYLOAD]] to [15 x i8]) }, section "llvm.metadata"
// CHECK-DAG: @[[ARGS_B:[^ ]+]] = private unnamed_addr constant { [15 x i8] } { [15 x i8] ptrtoint (ptr @[[PAYLOAD]] to [15 x i8]) }, section "llvm.metadata"
// CHECK: @llvm.global.annotations = appending global
// CHECK-SAME: { ptr @_ZN1C3fooEv, ptr @[[A_ANNOT]], ptr
// CHECK-SAME: ptr @[[ARGS_A]] }
// CHECK-SAME: { ptr @_ZN1C3barEv, ptr @[[B_ANNOT]], ptr
// CHECK-SAME: ptr @[[ARGS_B]] }