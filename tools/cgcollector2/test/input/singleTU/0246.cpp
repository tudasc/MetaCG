// main calls probe, so its definition and its noexcept-spec are instantiated for W<S>.
// The call to S::bar in the noexcept-spec is drawn as an edge from probe() to S::bar(), although the operand of
// noexcept is never evaluated

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
  W<S> n;
  n.probe();
  return 0;
}
