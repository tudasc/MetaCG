template <typename F>
void foo(F& f) {
  f();
}

template <typename F>
void foo_with_arg(F& f, int x) {
  f(x);
}

void bar() {}

void bar_with_arg(int x) {}

void caller(int x) {
  foo(bar);
  foo_with_arg(bar_with_arg, x);
}

int main() { caller(2); }
