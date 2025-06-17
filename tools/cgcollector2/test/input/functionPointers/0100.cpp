// function pointer without hit

int foo(float a) { return 0; }

int main(int argc, char* argv[]) {
  int (*function)();

  function();

  return 0;
}
