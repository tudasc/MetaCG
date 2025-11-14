
using func_t = void (*)();
void foo() {}
void bar() {}

int main() {
  func_t a, b;
  bool s = true;
  (s ? a : b) = foo;
  (s ? a : b)();
}
