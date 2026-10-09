// RUN: %clang -Xclang -load -Xclang %plugin -Xclang -add-plugin -Xclang cage-overrides-plugin %s -S -emit-llvm -o - | %FileCheck %s

struct A {
  virtual void operator()() {}
};

struct B : A {
  void operator()() override {}
};

int main() {
    A* a = new B();
    (*a)();
    return 0;
}

// CHECK-DAG: @[[A_OP_ANNOT:[^ ]+]] = private unnamed_addr constant [10 x i8] c"_ZN1AclEv\00"
// CHECK-DAG: @[[PAYLOAD:[^ ]+]] = private unnamed_addr constant [15 x i8] c"overridePlugin\00"
// CHECK-DAG: @[[ARGS:[^ ]+]] = private unnamed_addr constant { [15 x i8] } { [15 x i8] ptrtoint (ptr @[[PAYLOAD]] to [15 x i8]) }, section "llvm.metadata"
// CHECK: @llvm.global.annotations = appending global
// CHECK-SAME: { ptr @_ZN1BclEv, ptr @[[A_OP_ANNOT]], ptr
// CHECK-SAME: ptr @[[ARGS]] }