extern void foo();
extern void bar();
using FType = decltype(foo);

void f(FType n, FType k);
void f(FType n, FType k = bar);
void f(FType n = foo, FType k);

void work();

int main() {
  work();
  f();
}

void f(FType n, FType k) {
  n();
  k();
}
