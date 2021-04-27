
struct A {
  void foo() {}
};
void bar(A *a, void (A::*f)()) { (a->*f)(); }
int main() {
  A a;
  bar(&a, &A::foo);
}
