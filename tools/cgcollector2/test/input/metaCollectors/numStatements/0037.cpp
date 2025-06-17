
int foo(int k) {
  ++k;
  return 4 * k;
}

int main(int argc, char** argv) {
  int k = 12;
  int i = foo(k);

  do {
    i = k - 2;
    ++i;
  } while (i < k);

  return 0;
}
