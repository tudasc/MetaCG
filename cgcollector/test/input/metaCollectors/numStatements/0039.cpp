
int foo(int k) {
  ++k;
  return 4 * k;
}

int main(int argc, char **argv) {
  int k = 12;
  int i = foo(k);

  try {
    ++k;
  } catch (int ec) {
    ++i;
  }

  return 0;
}
