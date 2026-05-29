// Tests assignment of structs and function pointer members
extern void fooA();

typedef void (*Ftype)();

struct A {
  void* Member;
};

int main() {
  struct A First;
  First.Member = (void*)&fooA;
  struct A Second = First;
  ((Ftype)Second.Member)();
  return 0;
}
