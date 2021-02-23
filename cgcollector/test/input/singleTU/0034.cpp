
int foo(int k) {
  ++k;
  return 4 * k;
}

int main(int argc, char **argv) {
  int k = 12;
  int i = foo(k);

  switch (k) {
    default:
      k = 11;
      break;
  }

  return 0;
}
