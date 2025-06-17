struct A {
  virtual int foo() { return 42; }
};

struct D : public A {
  virtual int foo() override { return 1; }
};

void someFunc(A* a) { a->foo(); }

int main() {
  D d;
  someFunc(&d);
}
