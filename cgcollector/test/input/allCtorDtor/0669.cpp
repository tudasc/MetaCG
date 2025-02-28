// The destructor call of B must be inferred

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

