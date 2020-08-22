
struct A {
  virtual int foo() { return 42; }
};

struct D : public A {
  virtual int foo() override { return 1; }
};

struct Z : public D {
  virtual int foo() override { return 5; }
};

void someFunc(A *a) { a->foo(); }

int main() {
  Z z;
  someFunc(&z);
}
