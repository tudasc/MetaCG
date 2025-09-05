// Lambda function as expanded by cppinsights / as seen by the compiler

using func_t = int (*)(int);

int main() {
  using FuncPtr_4 = func_t;
  FuncPtr_4 a;

  class __lambda_5_7 {
   public:
    inline /*constexpr */ int operator()(int a) const { return a + 1; }

    using retType_5_7 = int (*)(int);
    inline /*constexpr */ operator retType_5_7() const noexcept { return __invoke; }

   private:
    static inline int __invoke(int a) { return a + 1; }

  } __lambda_5_7{};

  a = static_cast<int (*)(int)>(__lambda_5_7.operator __lambda_5_7::retType_5_7());
  int result;
  result = a(5);
}
