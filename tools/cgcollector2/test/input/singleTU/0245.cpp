// W can be instantiated with struct N, which has neither bar nor foo, because members of a class template are only
// instantiated when they are used. Neither probe's noexcept-spec nor get's body is instantiated for W<N>.
// Do not draw a call edge from probe() to nonexisting N::bar()

struct N {};

template <typename T>
struct W {
  T t;
  static int probe() noexcept(noexcept(T{}.bar())) { return 0; }
  int get() const { return t.foo(); }
};

int main() {
  W<N> n;
  return 0;
}
