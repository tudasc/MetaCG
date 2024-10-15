// call to overriding and overwritten functions

class MyClass {
 public:
  virtual void foo() {}
  virtual int bar() { return 0; }
};

class MyClassDerive : public MyClass {
 public:
  void foo() override;
  int bar() final;
};

void MyClassDerive::foo() {}
int MyClassDerive::bar() { return 1; }

void callsBar(MyClassDerive& mcd) { mcd.bar(); }

int main(int argc, char* argv[]) {
  MyClass mc;
  MyClassDerive mcd;
  mc.foo();
  mc.bar();

  callsBar(mcd);

  return 0;
}
