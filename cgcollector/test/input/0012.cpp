// call to redefining function

class MyClass {};

class MyClassDerive : public MyClass {
 public:
  int foo() { return 0; }
};

class MyClassDeriveD : public MyClassDerive {
 public:
  int foo() { return 1; }
};

int main(int argc, char *argv[]) {
  MyClassDeriveD mcdd;
  mcdd.foo();

  return 0;
}
