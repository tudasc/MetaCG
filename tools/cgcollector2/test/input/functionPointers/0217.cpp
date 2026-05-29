// Test passing a variable through a lambda that is cast to a fptr
using func_t = int (*)(int);

int main() {
  func_t lamb = [](int a) {
    int tmp = a;
    return tmp;
  };
  int parameter = 0;
  int result = lamb(parameter);
  return 0;
}
