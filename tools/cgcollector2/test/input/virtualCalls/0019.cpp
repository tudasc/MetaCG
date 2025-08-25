// call to overwritten function by pointer of base class

class MyClass {
 public:
  virtual void foo() {}
};

class MyClassDerive : public MyClass {
 public:
  void foo() override;
};

void MyClassDerive::foo() {}

int main(int argc, char* argv[]) {
  MyClass* mc;
  MyClassDerive mcd;
  mc = &mcd;
  mc->foo();

  return 0;
}
