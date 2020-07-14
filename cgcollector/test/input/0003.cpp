// main -> foo -> bar

int bar() { return 42; }

int foo() { return bar(); }

int main(int argc, char *argv[]) {
  foo();
  return 0;
}
