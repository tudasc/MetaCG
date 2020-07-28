
int one() {
  return 1;
}

int two() {
  return 2;
}

int three() {
  return 3;
}

int dflt() {
  return 42;
}

typedef int (*Fn) ();

Fn get(int i) { 
  if (i == 1) return one;
  if (i == 2) return two;
  if (i == 3) return three;
  return dflt;
}

int main() {
  int (*f)() = get(1);
  int a = f();

  return 0;
}

