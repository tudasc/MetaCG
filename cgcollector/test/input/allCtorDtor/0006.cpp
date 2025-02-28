// Tests if the destruction of temporary local variables is properly captured.

struct A {
  ~A() {}
};

struct B: A {
  ~B() { };
};

B makeB() {
  return B();
}

void foo() {
  makeB();
}

