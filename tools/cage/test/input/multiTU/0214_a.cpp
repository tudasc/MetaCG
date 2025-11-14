
using func_t = void (*)();
func_t g_foo;

void call_foo();
void foo() {}

int main() {
  g_foo = foo;
  call_foo();
}
