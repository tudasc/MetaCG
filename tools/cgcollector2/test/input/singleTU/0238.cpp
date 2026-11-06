// Calls inside a noexcept-spec that clang leaves uninstantiated point to the instantiated callees

struct S {
  int foo() const { return 1; }
  int bar() const { return 2; }
};

struct N {};

template <typename T>
struct W {
  T t;
  static int probe() noexcept(noexcept(T{}.bar())) { return 0; }
  int get() const { return t.foo(); }
};

int main() {
  W<S> w;
  W<N> n;
  return w.get();
}
