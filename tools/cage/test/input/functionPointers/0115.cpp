// Test for unions involving function pointers
extern void foo();
using FType = decltype(foo);

class C {
 public:
  union {
    int i;
    FType* f;
  };
};

int main() {
  C c;
  c.i = 4;
  c.f = foo;
  c.f();
}