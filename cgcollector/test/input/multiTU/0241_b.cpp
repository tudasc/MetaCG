using Ftype = void();

struct Base {
  virtual int foo();
  Ftype* f;
};

void calc(Base* arg) {
  //(*arg).foo();
  arg->foo();
  arg->f();
}
