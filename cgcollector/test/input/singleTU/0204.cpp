
using func_t = void (*)();
void call_func2(func_t f) {
  auto f2 = f;
  f();
}
void call_func(func_t f) {
  auto f2 = f;
  call_func2(f2);
}
void foo() {}
int main() {
  func_t f = foo;
  call_func(f);
  return 0;
}
