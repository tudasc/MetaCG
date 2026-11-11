// A call through a pointer to member function of class B can reach member functions of B and of its non-virtual
// bases, but neither member functions of unrelated classes, nor free functions, nor functions with other qualifiers
struct A {
  void inherited() {}
  void constQualified() const {}
};

struct B : A {
  void own() {}
};

struct V {
  void viaVirtualBase() {}
};

struct D : B, virtual V {};

struct Unrelated {
  void unrelated() {}
};

void freeFunction() {}

void callMember(B& b, void (B::*mp)()) { (b.*mp)(); }

void callConstMember(const A& a, void (A::*mp)() const) { (a.*mp)(); }

void callDerivedMember(D& d, void (D::*mp)()) { (d.*mp)(); }

int main() {
  D d;
  callMember(d, &B::own);
  callConstMember(d, &A::constQualified);
  callDerivedMember(d, &D::own);
  return 0;
}
