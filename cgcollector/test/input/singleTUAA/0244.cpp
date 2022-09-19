// Test that code to va related builtins does not crash. This does not check correct behavior of these functions.
void foo(const char *fmt, ...) {
  __builtin_va_list args;
  __builtin_va_start(args, fmt);
  __builtin_va_end(args);
}

int main() {
  const char *fmt = "Hello";
  foo(fmt);
  return 0;
}
