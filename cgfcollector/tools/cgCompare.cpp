#include <Callgraph.h>
#include <LoggerUtil.h>
#include <io/MCGReader.h>

static auto console = metacg::MCGLogger::instance().getConsole();
static auto errConsole = metacg::MCGLogger::instance().getErrConsole();

bool endsMatch(const std::string& a, const std::string& b) {
  if (a.size() >= b.size()) {
    return a.compare(a.size() - b.size(), b.size(), b) == 0;
  } else {
    return b.compare(b.size() - a.size(), a.size(), a) == 0;
  }
}

static bool compareNodesAndEdges(const metacg::Callgraph* cg1, const metacg::Callgraph* cg2) {
  bool equal = true;

  for (const auto& [id, node] : cg1->getNodes()) {
    if (!cg2->hasNode(node->getFunctionName())) {
      errConsole->error("Node {} is missing in the other call graph.", node->getFunctionName());
      equal = false;
      continue;
    }

    const auto& node2 = cg2->getNode(node->getFunctionName());
    if (node2->getHasBody() != node->getHasBody()) {
      errConsole->error("Node {} has different hasBody flags: expected {} got {}", node->getFunctionName(),
                        node2->getHasBody(), node->getHasBody());
      equal = false;
    }

    if (!endsMatch(node->getOrigin(), node2->getOrigin())) {
      errConsole->error("Node {} has different origins: expected '{}' got '{}'", node->getFunctionName(),
                        node2->getOrigin(), node->getOrigin());
      equal = false;
    }
  }

  for (const auto& [id, edge] : cg1->getEdges()) {
    auto name1 = cg1->getNode(id.first)->getFunctionName();
    auto name2 = cg1->getNode(id.second)->getFunctionName();

    if (!cg2->existEdgeFromTo(name1, name2)) {
      errConsole->error("Edge from {} to {} is missing in the other call graph.", name1, name2);
      equal = false;
    }
  }

  return equal;
}

int main(int argc, char* argv[]) {
  if (argc != 3) {
    errConsole->error("Usage: cgCompare <cg1> <cg2>");
    return EXIT_FAILURE;
  }

  metacg::io::FileSource fs1(argv[1]);
  metacg::io::FileSource fs2(argv[2]);

  auto mcgReader1 = metacg::io::createReader(fs1);
  auto mcgReader2 = metacg::io::createReader(fs2);
  if (!mcgReader1 || !mcgReader2) {
    return EXIT_FAILURE;
  }

  auto cg1 = mcgReader1->read();
  auto cg2 = mcgReader2->read();
  if (!cg1 || !cg2) {
    errConsole->error("Error reading call graphs.");
    return EXIT_FAILURE;
  }

  if (!compareNodesAndEdges(cg1.get(), cg2.get()) || !compareNodesAndEdges(cg2.get(), cg1.get())) {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
