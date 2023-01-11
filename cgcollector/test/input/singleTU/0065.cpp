/**
 * Testcase for a missing call in the load imbalance integration test
 */

typedef void (*Fn)();

void func1() {}

void func2() {}

Fn get_func_ptr(int i) {
  if (i == 0) {
    return func1;
  } else {
    return func2;
  }
}

/**
 * Main function
 */
int main(int argc, char **argv) {
  // function pointer test
  Fn func = get_func_ptr(0);
  func();
  return 0;
}
