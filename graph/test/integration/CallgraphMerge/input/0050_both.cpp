#include "0050.h"

struct DeriveTwo : public Base {
  virtual void foo() override {}
};

struct DeriveDerive : public Derive {
  virtual void foo() override {}
};