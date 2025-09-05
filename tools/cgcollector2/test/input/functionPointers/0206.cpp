
using func_t = void (*)();
void foo() {}
void bar() {}

void call() {
  func_t a, b;
  bool s = true;
  if (s)
    a = foo;
  else
    b = foo;
  if (s)
    a();
  else
    b();
}

int main() { call(); }
