#include <string>
#include <vector>

using edge = std::pair<std::string, std::string>;  // (caller, callee)

struct potentialFinalizer {
  std::size_t argPos;
  std::string procedureCalled;
  std::vector<edge> finalizerEdges;

  explicit potentialFinalizer(std::size_t pos, std::string procCalled)
      : argPos(pos), procedureCalled(std::move(procCalled)) {}

  void addFinalizerEdge(const edge& e) { finalizerEdges.emplace_back(e); }
};
