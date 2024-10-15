// function pointer with two hits

int foo(float a) { return 0; }

int hit1() { return 0; }

int hit2() { return 5; }

int main(int argc, char* argv[]) {
  int (*function)();

  function();

  return 0;
}
