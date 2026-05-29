// function pointer with two hits

void caller(int (*function)()) {}

void foo() {}

int hit() { return 5; }

int main(int argc, char* argv[]) {
  caller(&hit);
  foo();

  return 0;
}
