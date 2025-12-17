__attribute__((retain))
int boo() {
  int a = 1;
  int b = 0;
  return a + b;
}

__attribute__((retain))
int bar() { return boo(); }
