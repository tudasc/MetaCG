// Test for typedef for functions

void foo(int n) { int inner = n; }

typedef void (*Ftype)(int);

int main() {
  Ftype f = &foo;
  int arg = 5;
  f(arg);
  return 0;
}
