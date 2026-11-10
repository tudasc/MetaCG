// Calls a member function pointer that is returned by a function, without storing it in a variable first (compare 0209).

struct A {
  void foo() {}
  void bar() {}
  void baz() {}
};
using func_t = void (A::*)();
func_t get_f(bool b) {
  if (b) {
    return &A::bar;
  }
  return &A::foo;
}
int main() {
  A a;
  (a.*get_f(true))();
}
