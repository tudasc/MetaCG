// Tests the handling of builtin functions
extern void foo();

int main() {
  int x = 0;
  if (__builtin_expect(x, 0))
    foo();
  return 0;
}
