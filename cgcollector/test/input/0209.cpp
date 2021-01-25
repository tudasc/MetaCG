
struct A {
  void foo() {}
  void bar() {}
  void baz() {}
};
using func_t = void (A::*)();
func_t get_f(bool b) {
  func_t foo;
  foo = &A::foo;
  if (b) {
    return &A::bar;
  }
  return foo;
}
int main() {
  A a;
  auto f = get_f(1);
  (a.*f)();
}
