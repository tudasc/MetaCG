
int foo(int k) {
  ++k;
  return 4 * k;
}

int main(int argc, char** argv) {
  int k = 12;
  int i = foo(k);

  switch (k) {
    case 1:
      k = 2;
      break;
    case 4: {
      k = 100;
      i = 101;
      break;
    }
    default:
      k = 11;
      break;
  }

  return 0;
}
