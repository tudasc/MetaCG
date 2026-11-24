// The member function probe is never called and clang never tries to instantiate it
//

struct S {
  int foo() const { return 1; }
  int bar() const { return 2; }
};

template <typename T>
struct W {
  T t;
  static int probe() noexcept(noexcept(T{}.bar())) { return 0; }
  int get() const { return t.foo(); }
};

int main() {
  W<S> w;
  return w.get();
}
