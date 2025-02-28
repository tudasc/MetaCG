// The destructor ~A() should appear an (implicit) callee of ~B().

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

