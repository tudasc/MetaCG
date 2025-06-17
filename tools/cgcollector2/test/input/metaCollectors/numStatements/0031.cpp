
int foo(int k) {
  ++k;
  return 4 * k;
}

int main(int argc, char** argv) {
  int k = 12;
  int i = foo(k);

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
