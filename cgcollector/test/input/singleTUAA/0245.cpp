// Test that we can call a function and take the address of its return value
// Note the ground truth here contains some wrong edges, but this is caused by the traditional CG construction

int a;

class error_code {
 public:
  const int& category() const noexcept { return a; }
};

void foo(const error_code& __e) { (void*)&(__e.category()); }

int main() {}
