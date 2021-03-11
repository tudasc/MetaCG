
struct A {
  void foo() {}
};
void bar(A *a, void (A::*f)()) { (a->*f)(); }
void bar2(A *a, void (A::*f)()) { bar(a, f); }
int main() {
  A a;
  bar2(&a, &A::foo);
}
