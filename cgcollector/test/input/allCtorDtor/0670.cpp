struct A {
  ~A() {}
};

struct B: A {
  ~B() { };
};

void foo() {
  A* b = new B;
  // This should only call the destructor of A
  delete b;
}

