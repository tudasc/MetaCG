#include <iostream>
#include <vector>
// A LULESH inspired testcase.
class DataDomain {
 public:
  std::vector<double> fieldA;
  std::vector<double> fieldB;

  DataDomain(size_t size) : fieldA(size, 1.0), fieldB(size, 2.0) {}

  // Member functions to access data
  double getFieldA(size_t i) const { return fieldA[i]; }
  double getFieldB(size_t i) const { return fieldB[i]; }
};

int main() {
  using FieldPtr = double (DataDomain::*)(size_t) const;

  std::vector<FieldPtr> fieldPtrs = {&DataDomain::getFieldA, &DataDomain::getFieldB};

  size_t size = 5;
  DataDomain domain(size);

  std::vector<double> outputBuffer(size * fieldPtrs.size());

  double* destAddr = outputBuffer.data();

  for (FieldPtr field : fieldPtrs) {
    for (size_t i = 0; i < size; ++i) {
      destAddr[i] = (domain.*field)(i);
    }
    destAddr += size;
  }

  return 0;
}
