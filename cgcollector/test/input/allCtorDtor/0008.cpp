// Tests if implicit destruction of member variables is properly captured.

struct A {
  ~A() {}
};

struct B {
  // ~A() should be called here.
  ~B(){};
  A a;
};

void foo() {
  B* b = new B;
  delete b;
}
