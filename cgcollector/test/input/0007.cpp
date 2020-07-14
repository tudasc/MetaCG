// call to constructor and virtual destructor

class MyClass {
 public:
  MyClass() {}
  virtual ~MyClass() {}
};

int main(int argc, char *argv[]) {
  MyClass mc;

  return 0;
}
