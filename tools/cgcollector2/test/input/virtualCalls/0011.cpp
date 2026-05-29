// call to redefined function

class MyClass {};

class MyClassDerive : public MyClass {
 public:
  int foo() { return 0; }
};

class MyClassDeriveD : public MyClassDerive {
 public:
  int foo() { return 1; }
};

int main(int argc, char* argv[]) {
  MyClassDerive mcd;
  mcd.foo();

  return 0;
}
