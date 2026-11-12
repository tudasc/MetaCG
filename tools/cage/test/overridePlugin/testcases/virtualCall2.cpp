// RUN: %clang -Xclang -load -Xclang %plugin -Xclang -add-plugin -Xclang cage-overrides-plugin %s -S -emit-llvm -o - | %FileCheck %s

struct A {
  virtual void foo() {}
};

struct B : A {
  void foo() override {}
};

struct C : B {
  void foo() override {}
};

int main() {
    A* a = new B();
    a->foo();
    A* c = new C();
    c->foo();


    return 1;
}
// CHECK-DAG: @[[A_ANNOT:[^ ]+]] = private unnamed_addr constant [12 x i8] c"_ZN1A3fooEv\00", section "llvm.metadata"
// CHECK-DAG: @[[B_ANNOT:[^ ]+]] = private unnamed_addr constant [12 x i8] c"_ZN1B3fooEv\00", section "llvm.metadata"
// CHECK-DAG: @[[PAYLOAD:[^ ]+]] = private unnamed_addr constant [15 x i8] c"overridePlugin\00"

// CHECK-DAG: @[[ARGS:[^ ]+]] = private unnamed_addr constant { [15 x i8] } { [15 x i8] ptrtoint (ptr @[[PAYLOAD]] to [15 x i8]) }, section "llvm.metadata"
// CHECK-DAG: @[[ARGS2:[^ ]+]] = private unnamed_addr constant { [15 x i8] } { [15 x i8] ptrtoint (ptr @[[PAYLOAD]] to [15 x i8]) }, section "llvm.metadata"

// CHECK: @llvm.global.annotations = appending global
// CHECK-DAG: ptr @_ZN1B3fooEv, ptr @[[A_ANNOT]]
// CHECK-DAG: ptr @_ZN1C3fooEv, ptr @[[B_ANNOT]]
// CHECK-NOT: ptr @_ZN1C3fooEv, ptr @[[A_ANNOT]]
