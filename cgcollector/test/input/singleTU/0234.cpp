// Tests an explicit destructor call
typedef void (*Fn)();

void func1() {}

class X {
 public:
  Fn function;
  X(Fn arg) { function = arg; }
  ~X() { function(); }
};

int main() {
  const auto f = new X(func1);
  f->~X();
}