
#include "unistd.h"

#ifndef X
#define X 2
#endif

#ifndef Y
#define Y 4
#endif


int fooChild() {
  return 42;
}

int foo() {
  for(int i = 0; i < X; ++i) {
    sleep(Y);
  }
  return fooChild() * 3;
}

int bar() {
  sleep(Y);
  return 2;
}

int main(int argc, char **argv) {

  int a = foo();
  int b = bar();

  return 0;
}
