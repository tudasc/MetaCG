
using func_t = void (*)();

struct A {
  func_t f;
};

void foo() {}
func_t get_f(const A &a) { return a.f; }

int main() {
  A a;
  a.f = foo;
  auto f = get_f(a);
  f();
}
