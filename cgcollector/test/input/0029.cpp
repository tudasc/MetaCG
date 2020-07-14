

int main(int argc, char **argv) {
  int k = 12;
  int i = 0;

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
