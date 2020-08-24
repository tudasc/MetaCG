
class MyClass {
 public:
  virtual int foo() = 0;
};

class MyClassDerive : public MyClass {
 public:
  virtual int foo();
};
