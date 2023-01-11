
using func_t = void (*)();

struct A {
  func_t f;
};

void foo() {}
void call_f(const A &arg) { arg.f(); }

int main() {
  A a;
  a.f = foo;
  call_f(a);
}
