typedef void (*func_t)();

void foo(func_t fp) {}
void bar() {}

int main(int argc, char **argv) {
  func_t f;
  f = bar;
  foo(f);
  return 0;
}
