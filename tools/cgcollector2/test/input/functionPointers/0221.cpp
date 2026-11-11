// A call through a pointer to member function that is not stored in a variable
struct A {
  void foo() {}
  void bar() {}
};

struct Other {
  void baz() {}
};

void (A::*getPtr(bool b))() { return b ? &A::foo : &A::bar; }

int main() {
  A a;
  (a.*getPtr(true))();
  return 0;
}
