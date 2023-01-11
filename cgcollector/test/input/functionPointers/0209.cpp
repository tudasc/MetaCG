
struct A {
  void foo() {}
  void bar() {}
  void baz() {}
};
using func_t = void (A::*)();
func_t get_f(bool b) {
  func_t fooPtr;
  fooPtr = &A::foo;
  if (b) {
    return &A::bar;
  }
  return fooPtr;
}
int main() {
  A a;
  auto f = get_f(1);
  (a.*f)();
}
