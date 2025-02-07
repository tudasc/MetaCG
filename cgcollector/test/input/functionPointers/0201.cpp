
using func_t = void (*)();

void foo() {};

func_t get_f(func_t f) { return f; }

int main() {
  func_t f = get_f(foo);
  f();
}
