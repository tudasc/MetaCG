void foo() {}

int bar() {
  foo();
  return 0;
}

void notcalled() { bar(); }

int main(int argc, char **argv) {
  foo();
  bar();
  return 0;
}
