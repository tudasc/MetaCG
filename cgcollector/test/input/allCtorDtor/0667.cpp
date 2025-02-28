// The destructor ~A() does not appear as a callee of ~B().

struct A {
  ~A() {}
};

struct B: A {
  ~B() { };
};

void foo() {
  B* b = new B;
  delete b;
}

