
#include "unistd.h"

int foo() {
  sleep(10);
  return 0;
}

int main(int argc, char **argv) {
  int a = foo();
  return 0;
}
