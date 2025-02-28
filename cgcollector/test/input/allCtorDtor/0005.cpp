// Tests if the destruction of local variables is properly captured.

struct A {
  ~A() {}
};

struct B: A {
  ~B() { };
};

void foo() {
  B b;
}

