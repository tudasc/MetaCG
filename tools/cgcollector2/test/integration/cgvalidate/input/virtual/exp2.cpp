struct A {
  virtual int foo() { return 42; }
};

struct D : public A {};

void someFunc(D* d) { d->foo(); }

int main() {
  D d;
  someFunc(&d);
}
