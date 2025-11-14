// Test fpr a lambda that is casted to a function pointer
using func_t = void (*)();

int main() {
  func_t a, b;
  bool s = true;
  (s ? a : b) = []() {};
  (s ? a : b)();
  return 0;
}
