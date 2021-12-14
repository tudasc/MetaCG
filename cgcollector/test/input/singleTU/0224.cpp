int test2() { return 0; }

int test() {
  do {
    return test2();
  } while (true);
};

int main() {
  while (true) {
    int a;
    while (true) {
      test();
      break;
    }
    a = 0;
    break;
  }
  test();
  return 0;
}