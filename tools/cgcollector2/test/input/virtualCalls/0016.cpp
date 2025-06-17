// override multiple functions

class MyClass {
 public:
  virtual void foo() {}
};

class MyClass2 {
 public:
  virtual void foo() {}
};

class MyClassDerive : public MyClass, public MyClass2 {
 public:
  void foo() override;
};

void MyClassDerive::foo() {}

int main(int argc, char* argv[]) {
  MyClass mc;
  MyClassDerive mcd;
  mc.foo();

  return 0;
}
