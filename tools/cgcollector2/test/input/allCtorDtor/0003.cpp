// Member b of A is default-initialized, this must be caught by cgcollector.
// This will lead to a callpath existing from A() through B() to hidden()

void hidden() {}

struct B {
  int* data;
  B() { hidden(); }
};

struct A {
  A() {}

  B b;
};
