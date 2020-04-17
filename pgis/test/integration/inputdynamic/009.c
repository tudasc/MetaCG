
#include "unistd.h"

int fooLeaf() {
  return 4;
}

int fooChildTwo() {
  return (fooLeaf() * 2) / fooLeaf();
}

int fooChildOne() {
  int a = 0;
  int b = fooLeaf();
  int c = 2;
  a++;
  return (a * b) / c;
}

int fooIntermediate() {
  sleep(8);
  return fooChildOne() / fooChildTwo();
}


int foo() {
  sleep(10);
  fooIntermediate();
  return 0;
}

int bar() {
  sleep(1);
  return 0;
}

int main(int argc, char **argv) {
  int a = foo();
  int b = bar();
  return 0;
}
