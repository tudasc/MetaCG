
struct Base {
  virtual void foo() {}
};

struct Derive : public Base {
  virtual void foo() override {}
};
