// call to constructor and virtual destructor

class MyClass {
 public:
  MyClass(int b) { int c = b; }
  virtual ~MyClass() {}
};

int main(int argc, char* argv[]) {
  int a = 42;
  MyClass mc(a);

  return 0;
}
