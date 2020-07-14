
#include "unistd.h"

int fooChildTwo() {
  int a = 0;
  int b = 4;
  int c = 2;
  return (a * b) / c;
}

int fooChildOne() {
  int a = 0;
  int b = 4;
  int c = 2;
  a++;
  a = 0;
  b = 4;
  c = 2;
  a++;
  a++;
  a++;
  a++;
  a++;
  a++;
  a++;
  a++;
  a++;
  a++;
  a++;
  a++;
  a++;
  a++;
  a++;
  a++;
  a++;
  a++;
  a++;
  a++;
  a++;
  a++;
  a++;
  a++;
  a++;
  a++;
  a++;
  a++;
  a++;
  a++;
  return (a * b) / c;
}


int foo() {
  sleep(10);
  fooChildTwo();
  fooChildOne();
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
