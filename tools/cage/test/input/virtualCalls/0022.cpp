// Test for checking that functions in uninstantiated templates do not get included into the callgraph and to check the
// name mangling of virtual functions in classes that depend on template parameters.
template <typename T>
class BaseUnused {
 public:
  virtual void bad1() {}
};

template <typename U>
class ChildUnused : public BaseUnused<U> {
 public:
  virtual void bad2() {}
};

template <typename T>
class BaseUsed {
 public:
  virtual void good1() {}
};

template <typename U>
class ChildUsed : public BaseUsed<U> {
 public:
  virtual void good2() {}
};

int main() {
  ChildUsed<int> var;
  var.good1();
  var.good2();
  return 0;
}