// Tests different combinations of function pointers, temporaries and constructors

extern int fooA();
extern int fooB();
extern int fooC();
extern int fooD();

using FType = decltype(fooA);

class A {
 public:
  FType *function;
  A() { function = fooA; }
};

class B {
 public:
  FType *function;
  B() { function = fooB; }
};

class C {
 public:
  FType *function;
  C() { function = fooC; }
};

class D {
 public:
  FType *function;
  D() { function = fooD; }
};

void callA() { (A()).function(); }

B helperB() { return B(); }

void callB() { helperB().function(); }

void callC(C c) { c.function(); }

D helperD() { return {}; }

void callD() { helperD().function(); }

int main() {
  callA();
  callB();
  callC(C());
  callD();
}
