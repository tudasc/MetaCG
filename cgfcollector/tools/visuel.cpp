#include <Callgraph.h>
#include <DotIO.h>
#include <LoggerUtil.h>
#include <fstream>
#include <io/MCGReader.h>

static auto console = metacg::MCGLogger::instance().getConsole();
static auto errConsole = metacg::MCGLogger::instance().getErrConsole();

int main(int argc, char* argv[]) {
  if (argc != 2) {
    errConsole->error("Usage: visuel <cg>");
    return EXIT_FAILURE;
  }

  metacg::io::FileSource fs1(argv[1]);

  auto mcgReader1 = metacg::io::createReader(fs1);
  if (!mcgReader1) {
    return EXIT_FAILURE;
  }

  auto cg = mcgReader1->read();
  if (!cg) {
    errConsole->error("Error reading call graphs.");
    return EXIT_FAILURE;
  }

  metacg::io::dot::DotGenerator dotGen(cg.get());
  dotGen.generate();

  std::ofstream outFile("callgraph.dot");
  if (!outFile.is_open()) {
    errConsole->error("Error opening output file for writing.");
    return EXIT_FAILURE;
  }
  outFile << dotGen.getDotString();

  outFile.close();

  return EXIT_SUCCESS;
}
