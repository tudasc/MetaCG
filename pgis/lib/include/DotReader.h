
#include "Callgraph.h"

#include <fstream>
#include <string>

#ifndef DOTREADER_H_
#define DOTREADER_H_

/**
 * A not so robust reader for dot-files
 * \author roman
 */
namespace DOTCallgraphBuilder {

std::string extractBetween(const std::string &s, const std::string &pattern, size_t &start) {
  size_t first = s.find(pattern, start) + pattern.size();
  size_t second = s.find(pattern, first);

  start = second + pattern.size();
  return s.substr(first, second - first);
}

void build(std::string filePath, Config *c) {
  //CallgraphManager *cg = new CallgraphManager(c);
  auto &cg = CallgraphManager::get();

  std::ifstream file(filePath);
  std::string line;

  while (std::getline(file, line)) {
    if (line.find('"') != 0) {  // does not start with "
      continue;
    }

    std::cout << line << std::endl;

    if (line.find("->") != std::string::npos) {
      if (line.find("dotted") != std::string::npos) {
        continue;
      }

      // edge
      size_t start = 0;
      std::string parent = extractBetween(line, "\"", start);
      std::string child = extractBetween(line, "\"", start);

      size_t numCallsStart = line.find("label=") + 6;
      unsigned long numCalls = stoul(line.substr(numCallsStart));

      // filename & line unknown; time already added with node
      cg.putEdge(parent, "", -1, child, numCalls, 0.0, 0, 0);

    } else {
      // node
      size_t start = 0;
      std::string name = extractBetween(line, "\"", start);
      double time = stod(extractBetween(line, "\\n", start));

      cg.findOrCreateNode(name, time);
    }
  }
  file.close();
}
};  // namespace DOTCallgraphBuilder

#endif
