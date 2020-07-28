
int one() {
  return 1;
}

typedef int (*Fn) ();

Fn get(int i) {
  Fn f = one;
  return f;
}

Fn get2(int i) {
  return get(i);
}

int main() {

  get2(1)();
  return 0;
}

