
int foo() { return 4 + 2; }

int main(int argc, char** argv) {
  int k = 12;
  int i = foo();

  while (i < k) {
    if (k % 2 == 0) {
      ++k;
    } else {
      ++i;
      ++k;
    }
    ++i;
  }

  return 0;
}
