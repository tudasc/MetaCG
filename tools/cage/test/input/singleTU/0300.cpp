// Checking that the call target overapproximation is not used on already resolved virtual calls.
struct Base {
  virtual void foo() {};
  virtual void bar() {};
};

struct Derived: public Base {
  void foo() override {}
};

int main() {
  Base* b = new Derived;
  b->foo();
  return 0;
}