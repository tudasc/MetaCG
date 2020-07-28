
int one() { return 1; }

int dflt() { return 42; }

typedef int (*Fn)();

Fn get(int i) {
  if (i == 1)
    return one;
  return dflt;
}

Fn get2(int i) { return get(i); }

int main() {
  int (*f)() = get2(1);
  int a = f();

  return 0;
}
