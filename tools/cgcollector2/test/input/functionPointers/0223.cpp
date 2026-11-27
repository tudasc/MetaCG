// A call through a pointer to member function only reaches member functions with the same ref-qualifier and
// cv-qualifiers, as both are part of the type of the implicit object parameter
struct A {
  void unqualified() {}
  void lvalue() & {}
  void rvalue() && {}
  void constLvalue() const& {}
  void constRvalue() const&& {}
};

struct B : A {
  void ownLvalue() & {}
  void ownRvalue() && {}
};

void callUnqualified(A& a, void (A::*mp)()) { (a.*mp)(); }

void callLvalue(A& a, void (A::*mp)() &) { (a.*mp)(); }

void callRvalue(A&& a, void (A::*mp)() &&) { (static_cast<A&&>(a).*mp)(); }

void callConstLvalue(const A& a, void (A::*mp)() const&) { (a.*mp)(); }

void callConstRvalue(const A&& a, void (A::*mp)() const&&) { (static_cast<const A&&>(a).*mp)(); }

// Members of the non-virtual base A are reachable, but only with the same ref-qualifier
void callDerivedRvalue(B&& b, void (B::*mp)() &&) { (static_cast<B&&>(b).*mp)(); }

// A pointer to member function that is not stored in a variable
void (A::*getLvaluePtr())() & { return &A::lvalue; }

void callReturnedLvalue(A& a) { (a.*getLvaluePtr())(); }

int main() {
  A a;
  B b;
  callUnqualified(a, &A::unqualified);
  callLvalue(a, &A::lvalue);
  callRvalue(static_cast<A&&>(a), &A::rvalue);
  callConstLvalue(a, &A::constLvalue);
  callConstRvalue(static_cast<const A&&>(a), &A::constRvalue);
  callDerivedRvalue(static_cast<B&&>(b), &B::ownRvalue);
  callReturnedLvalue(a);
  return 0;
}
