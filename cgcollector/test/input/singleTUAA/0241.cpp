// Test that we can pass function pointers with an -> member access

extern int foo();

using FType = decltype(foo);

struct Storage {
  FType *fptr;
};

int main() {
  Storage s;
  s.fptr = &foo;
  auto sptr = &s;
  auto f = sptr->fptr;
  f();
  return 0;
}