// Tests passing function pointers as C++ style constructor init

typedef void (*Function)();

void foo() {}

__attribute__((retain))
void bar() {}

class C {
 public:
  C(Function F) : member(F) {};

  Function member;
};

int main() {
  C a(&foo);
  Function b = a.member;
  b();
}