// 2 overrides in a row
// but no multiple inheritanc

class MyClass {
 public:
  virtual void foo() {}
};

class MyClass2 : public MyClass {
 public:
  virtual void foo() override {}
};

class MyClassDerive : public MyClass2 {
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
