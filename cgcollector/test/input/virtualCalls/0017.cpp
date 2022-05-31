// override multiple functions
// override function of super super class

class MyClass {
 public:
  virtual void foo() {}
};

class MyClass2 {
 public:
  virtual void foo() {}
};

class MyClassStep : public MyClass2 {};

class MyClassDerive : public MyClass, public MyClassStep {
 public:
  void foo() override;
};

void MyClassDerive::foo() {}

int main(int argc, char *argv[]) {
  MyClass mc;
  MyClassDerive mcd;
  mc.foo();

  return 0;
}
