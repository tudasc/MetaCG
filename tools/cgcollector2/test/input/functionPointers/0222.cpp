// A call through an ordinary function pointer can not reach non-static member functions with the same signature,
// but it can reach static member functions
struct A {
  void member() {}
  static void staticMember() {}
};

void freeFunction() {}

void callPointer(void (*fp)()) { fp(); }

int main() {
  callPointer(&freeFunction);
  return 0;
}
