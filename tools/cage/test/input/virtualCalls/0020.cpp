// call to overwritten function by pointer of current class

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
  MyClassDerive* mc;
  MyClassDerive mcd;
  mc = &mcd;
  mc->foo();

  return 0;
}
