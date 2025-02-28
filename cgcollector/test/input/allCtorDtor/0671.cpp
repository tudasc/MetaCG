struct A {
  ~A() {}
};

struct B {
  ~B() { }; // ~A() should be called here.
  A a;
};

void foo() {
  B* b = new B;
  delete b;
}

