
using func_t = void (*)();

void get_func2(func_t* out, func_t in) {
  *out = in;
  in();
}
func_t get_func(func_t f) {
  func_t res;
  get_func2(&res, f);
  return res;
}
void foo() {}
int main() {
  auto f = get_func(foo);
  f();
}
