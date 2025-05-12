struct Object {
 public:
  virtual void doSomething() {}
  Object() {}
  virtual ~Object() = default;
};

struct Derived : public Object {
 public:
  void doSomething() override {}
  Derived() {}
  virtual ~Derived() {}
};

struct Derived2 : public Derived {
  void doSomething() override {}
  Derived2() {}
  virtual ~Derived2() {}
};

int main() {
  Object* obj = new Derived2();
  obj->doSomething();

  delete obj;

  return 0;
}
