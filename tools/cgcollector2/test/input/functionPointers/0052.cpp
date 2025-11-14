
int one() { return 1; }

int dflt() { return 42; }

typedef int (*Fn)();

Fn get(int i) {
  Fn a;
  a = dflt;
  if (i == 1) {
    return one;
  }
  return a;
}

Fn get2(int i) { return get(i); }

int main() {
  get2(1)();
  return 0;
}
