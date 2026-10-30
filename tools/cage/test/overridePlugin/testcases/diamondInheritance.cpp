// RUN: %clang -Xclang -load -Xclang %plugin -Xclang -add-plugin -Xclang cage-overrides-plugin %s -S -emit-llvm -o - | %FileCheck %s

struct A {
  virtual void foo() {}
};

struct B : virtual A {
  void foo() override {}
};

struct C : virtual A {
  void foo() override {}
};

struct D : B, C {
  void foo() override {}
};

int main() {
    D* d = new D();
    d->foo();
    return 0;
}

// CHECK-DAG: @[[B_ANNOT:[^ ]+]] = private unnamed_addr constant [12 x i8] c"_ZN1B3fooEv\00"
// CHECK-DAG: @[[C_ANNOT:[^ ]+]] = private unnamed_addr constant [12 x i8] c"_ZN1C3fooEv\00"
// CHECK-DAG: @[[PAYLOAD:[^ ]+]] = private unnamed_addr constant [15 x i8] c"overridePlugin\00"
// CHECK-DAG: @[[ARGS:[^ ]+]] = private unnamed_addr constant { [15 x i8] } { [15 x i8] ptrtoint (ptr @[[PAYLOAD]] to [15 x i8]) }, section "llvm.metadata"
// CHECK: @llvm.global.annotations = appending global
// CHECK-SAME: { ptr @_ZN1D3fooEv, ptr @[[B_ANNOT]],
// CHECK-SAME: { ptr @_ZN1D3fooEv, ptr @[[C_ANNOT]],
// CHECK-NOT: @_ZN1D3fooEv, ptr @.str,