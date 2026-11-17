// Stack allocated class template instances capture their user-provided constructors, which construct the member
// of template parameter type. Calls inside a noexcept-spec that clang leaves uninstantiated are pruned, as nothing
// calls probe

struct S {
  int foo() const { return 1; }
  int bar() const { return 2; }
};

struct N {};

template <typename T>
struct W {
  T t;
  W() {}
  static int probe() noexcept(noexcept(T{}.bar())) { return 0; }
  int get() const { return t.foo(); }
};

int main() {
  W<S> w;
  W<N> n;
  return w.get();
}
