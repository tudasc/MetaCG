
int *k;

int foo(int k) {
  if (k == 42) {
    return 0;
  }
  return foo(k);
}

int main() {
  foo(42);
  return 0;
}
