// Test branch

int main() {
  bool a = 5 ? false : true;
  int b = 7;
  if (a) {
    switch (b) {
      default:
        break;
    }
  } else if (b) {
    bool f = true;
    while (f) {
      f = false;
    }
  }

  return 0;
}