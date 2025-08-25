int foo(int arg[], int offset) { return arg[offset]; }

struct A {
  int a;
  int b;
};

int boo(struct A a) { return a.b;
}