// Test that we can pass parameters to function that are defined with __typeof. The problem here is that the
// RecursiveASTVisitor is not visiting their parameters
namespace test_cxx {
extern "C" bool uselocale2(bool);
extern "C" __typeof(uselocale2) __uselocale2;

}  // namespace test_cxx

int main() {
  bool i = false;
  test_cxx::__uselocale2(i);
  return 0;
}
