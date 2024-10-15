
int bar(int f) {
  int k = f % 3;
  int n = 0;
  while (k > 0) {
    --k;
    ++n;
  }
  return n;
}

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

  int n = bar(i);

  return 0;
}
