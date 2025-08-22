// Tests the handing of functions with different but compatible definitions
void foo(int n) { int var = n; }
typedef void (*Ftype)(int);

void baa(Ftype f, int n, int, int arg);

int main() {
  Ftype f = &foo;
  int arg = 5;
  int arg2 = 6;
  int arg3 = 7;
  baa(f, arg, arg2, arg3);
  return 0;
}

void baa(Ftype f, int n, int a, int b) { f(n + a + b); }
