
using func_t = void (*)();

func_t f0(func_t f) { return f; }
func_t f1(func_t f) { return f0(f); }
func_t f2(func_t f) { return f1(f); }
func_t f3(func_t f) { return f2(f); }
func_t f4(func_t f) { return f3(f); }
func_t f5(func_t f) { return f4(f); }
func_t f6(func_t f) { return f5(f); }
func_t f7(func_t f) { return f6(f); }
func_t f8(func_t f) { return f7(f); }
func_t f9(func_t f) { return f8(f); }

void foo() {}

int main() {
  auto f = f9(foo);
  f();
}
