// Tests if non-virtual destructors are handled correctly.

struct A {
  ~A() {}
};

struct B : A {
  ~B(){};
};

void foo() {
  A* b = new B;
  // This should only call the destructor of A
  delete b;
}
