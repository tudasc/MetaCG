// Test for the handling of member functions and members that hold function pointers
class A {
 public:
  void* member;
  void* memberFunction() {
    auto p = this;
    return member;
  }
};

void f() {}
typedef void (*Fn)();

int main() {
  A a;
  a.member = (void*)&f;
  Fn b = (Fn)a.memberFunction();
  b();
  return 0;
}