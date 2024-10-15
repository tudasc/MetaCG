// call to overwritten function by pointer of middle class

class MyClass {
 public:
  virtual void foo() {}
};

class MyClassDerive : public MyClass {
 public:
  void foo() override;
};

class MyClassDeriveDerive : public MyClassDerive {
 public:
  void foo() override{};
};

void MyClassDerive::foo() {}

int main(int argc, char* argv[]) {
  MyClassDerive* mc;
  MyClassDeriveDerive mcd;
  mc = &mcd;
  mc->foo();

  return 0;
}
