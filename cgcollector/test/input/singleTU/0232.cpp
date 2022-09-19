// Test for the handling of new and delete and calling function pointers within them
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
  delete f;
}