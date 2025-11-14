

int main(int argc, char** argv) {
  int k = 12;

  for (int i = 0; i < k;) {
    if (k % 2 == 0) {
      ++k;
    } else {
      ++i;
      ++k;
    }
  }

  return 0;
}
