// RUN: %cgpatchcxx clang++ %s -emit-llvm -S -o - | %filecheck %s

inline int inlined_add(int x, int y) { return x + y; }

void caller() { inlined_add(96, 4); }

// CHECK: define linkonce_odr dso_local noundef i32 @_Z11inlined_addii(
// CHECK: declare void @__metacg_indirect_call(
