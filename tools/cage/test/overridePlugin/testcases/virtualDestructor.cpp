// RUN: %clang -Xclang -load -Xclang %plugin -Xclang -add-plugin -Xclang cage-overrides-plugin %s -S -emit-llvm -o - | %FileCheck %s

struct A {
  virtual ~A() {}
};

struct B : A {
  ~B() override {}
};

int main() {
    A* a = new B();
    delete a;
    return 0;
}

// CHECK-DAG: @[[A_DTOR_ANNOT:[^ ]+]] = private unnamed_addr constant [10 x i8] c"_ZN1AD0Ev\00"
// CHECK-DAG: @[[PAYLOAD:[^ ]+]] = private unnamed_addr constant [15 x i8] c"overridePlugin\00"
// CHECK-DAG: @[[ARGS:[^ ]+]] = private unnamed_addr constant { [15 x i8] } { [15 x i8] ptrtoint (ptr @[[PAYLOAD]] to [15 x i8]) }, section "llvm.metadata"
// CHECK: @llvm.global.annotations = appending global
// CHECK-SAME: { ptr @_ZN1BD0Ev, ptr @[[A_DTOR_ANNOT]], ptr
// CHECK-SAME: ptr @[[ARGS]] }
// CHECK-SAME: { ptr @_ZN1BD2Ev, ptr @[[A_DTOR_ANNOT]], ptr
// CHECK-SAME: ptr @[[ARGS]] }