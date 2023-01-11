
using func_t = void (*)();

int main() {
  func_t a;
  a = [](){};
  a();
  return 0;
}
