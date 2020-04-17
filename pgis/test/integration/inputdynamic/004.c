
#include "unistd.h"

int foo() {
  sleep(10);
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
