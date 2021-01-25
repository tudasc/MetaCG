
using func_t = void (*)();

void foo() {}

void cast() {
  func_t f;
  f = reinterpret_cast<func_t>(reinterpret_cast<void *>(foo));
  f();
}
