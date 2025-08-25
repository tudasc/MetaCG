// call to overriding function

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
  MyClass mc;
  MyClassDerive mcd;
  mcd.foo();

  return 0;
}
