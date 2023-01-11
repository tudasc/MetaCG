/**
 * Testcase for the multi TU imbalance integration test
 */
#define NULL 0

void get_func_ptr(void (**func)(), int i);

void function_pointer_test() {
  void (*r)() = NULL;

  get_func_ptr(&r, 0);

  r();
}

/**
 * Main function
 */
int main(int argc, char **argv) {
  function_pointer_test();
  return 0;
}
