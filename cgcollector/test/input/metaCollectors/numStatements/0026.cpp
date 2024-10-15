

int main(int argc, char** argv) {
  int k = 12;

  for (int i = 0; i < k;) {
    ++i;
    --k;
    if (k % 2 == 0) {
      ++k;
    }
  }

  return 0;
}
