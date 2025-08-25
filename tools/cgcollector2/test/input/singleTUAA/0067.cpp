// Test for the alias analysis to check that it can generate unique USRs when macros are involved
int foo(int a) { return 5 + a; }

#define TWICE(a)                                                                                                       \
  foo(a);                                                                                                              \
  foo(a + 1);

#define MY_MAX(a, b) (((a) < (b)) ? (b) : (a))

int main() {
  TWICE(5);
  return MY_MAX(8, foo(2));
}
