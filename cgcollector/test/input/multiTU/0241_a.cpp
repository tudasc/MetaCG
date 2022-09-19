// Test for handling a combination of function pointers and virtual classes across multiple TUs
extern void f1();
extern void f2();

using Ftype = void();

struct Base {
  virtual int foo() {
    f = nullptr;
    return 1;
  }
  Ftype *f;
};

struct Child1 : Base {
  int foo() override {
    f = f1;
    return 2;
  }
};

struct Child2 : Base {
  int foo() override {
    f = f2;
    return 3;
  }
};

void calc(Base *arg);

int main() {
  Base *b = new Base();
  bool test1 = true;
  bool test2 = true;
  if (test1) {
    b = new Child1();
  }
  if (test2) {
    b = new Child2();
  }
  calc(b);
}
