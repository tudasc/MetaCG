
int* k;

void *malloc(int n) {
  return (void *) n;
}

int main() {
  int j =23;
  int *i = (int *)malloc(8);
  i = (int *)malloc(42);
  k = new int[j];
  return 0;
}
