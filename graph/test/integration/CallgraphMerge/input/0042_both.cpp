int foo() { return 42; }

int baz() { return foo(); }

int boo() {
  int a = 1;
  int b = 0;
  return a + b;
}

int bar() { return boo(); }