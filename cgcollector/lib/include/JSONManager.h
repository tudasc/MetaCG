#ifndef CGCOLLECTOR_JSONMANAGER_H
#define CGCOLLECTOR_JSONMANAGER_H

#include "nlohmann/json.hpp"

#include <fstream>
#include <iostream>
#include <unordered_set>

using FunctionNames = std::unordered_set<std::string>;

inline void readIPCG(const std::string &filename, nlohmann::json &callgraph) {
  std::ifstream file(filename);
  if (!file.is_open()) {
    std::cerr << "[Error] Unable to open inputfile " << filename << std::endl;
    exit(-1);
  }
  file >> callgraph;
}

inline void writeIPCG(const std::string &filename, const nlohmann::json &callgraph) {
  std::ofstream file(filename);
  if (!file.is_open()) {
    std::cerr << "[Error] Unable to open outputfile " << filename << std::endl;
    exit(-1);
  }
  file << callgraph;
}

inline void insertNode(nlohmann::json &callgraph, const std::string &nodeName, const FunctionNames &callees,
                       const FunctionNames &callers, const FunctionNames &overriddenBy,
                       const FunctionNames &overriddenFunctions, const bool isVirtual, const bool doesOverride,
                       const bool hasBody) {
  callgraph[nodeName] = {{"callees", callees},
                         {"isVirtual", isVirtual},
                         {"doesOverride", doesOverride},
                         {"overriddenFunctions", overriddenFunctions},
                         {"overriddenBy", overriddenBy},
                         {"parents", callers},
                         {"hasBody", hasBody}};
}

inline void insertDefaultNode(nlohmann::json &callgraph, const std::string &nodeName) {
  const FunctionNames empty;
  insertNode(callgraph, nodeName, empty, empty, empty, empty, false, false, false);
}

#endif /* ifndef CGCOLLECTOR_JSONMANAGER_H */
