// no call
// but a second function

__attribute__((retain))
int foo() {
  int a = 0;
  for (int i = 0; i < 5; ++i) {
    a += 2;
  }
  return a;
}

int main(int argc, char* argv[]) { return 0; }
