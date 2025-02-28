// The destructor call of B must be inferred

struct A {
  ~A() {}
};

struct B: A {
  ~B() { };
};

void foo() {
  B b;
}

