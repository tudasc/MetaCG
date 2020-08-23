
typedef void (*target_func_T)(int a, int b, int c);

void callTarget(int a, int b, int c) {
  int d = a + b;
  d *= c;
}

void pointerCaller(target_func_T f, int a, int b, int c) { f(a, b, c); }

int main(int argc, char **argv) {
  int a, b, c;
  a = b = c = 42;
  pointerCaller(callTarget, a, b, c);

  return 0;
}
