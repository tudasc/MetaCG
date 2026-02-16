#include "0050.h"

struct DeriveTwo : public Base {
  virtual void foo() override {}
};

int main() {
  Base* b = new Derive;
  b->foo();
  return 0;
}