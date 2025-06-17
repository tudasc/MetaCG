
using func_t = void (*)();

struct A {
  func_t f;
};

void foo() {}

int main() {
  A a;
  a.f = foo;
  a.f();
}
