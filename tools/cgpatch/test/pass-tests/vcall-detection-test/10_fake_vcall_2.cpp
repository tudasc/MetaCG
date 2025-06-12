// RUN: %cgpatchcxx --verbose clang++ %s %cppargs -emit-llvm -S | %filecheck %s

typedef void (**fake_vtable_t)();

extern fake_vtable_t get_fake_vtable();

int main() {
  auto fake_vtable = get_fake_vtable();
  fake_vtable[0]();

  // CHECK: Traversing function: main
  // CHECK-NOT: Virtual call identified
  // CHECK: Call is other indirect call

  return 0;
}
