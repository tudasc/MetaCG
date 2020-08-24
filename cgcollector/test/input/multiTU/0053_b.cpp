// call to redefining function

#include "0053.h"

class MyClassDeriveD : public MyClassDerive {
 public:
  int foo() { return 1; }
};

int main(int argc, char *argv[]) {
  MyClass *m = new MyClassDeriveD();
  m->foo();

  return 0;
}
