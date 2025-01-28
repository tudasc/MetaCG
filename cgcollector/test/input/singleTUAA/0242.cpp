// Test for the handling of constructors and destructors when passing arguments
struct A {
  ~A() {};
};

// Should not call destructors
void foo(A a) {}

// Needs to call destructor for A
void boo(A& a) { foo(a); }

int main() {
  A o;
  boo(o);
}
