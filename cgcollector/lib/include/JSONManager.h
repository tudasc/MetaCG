#ifndef CGCOLLECTOR_JSONMANAGER_H
#define CGCOLLECTOR_JSONMANAGER_H

#include "MetaCollector.h"

#include "CallGraph.h"

#include "nlohmann/json.hpp"

#include <unordered_set>

using FunctionNames = std::unordered_set<std::string>;

void convertCallGraphToJSON(const CallGraph &cg, nlohmann::json &j);

void addMetaInformationToJSON(nlohmann::json &j, const std::string &metaInformationName,
                              const std::unordered_map<std::string, std::unique_ptr<MetaInformation>> &meta);

void insertDefaultNode(nlohmann::json &callgraph, const std::string &nodeName);

void insertNode(nlohmann::json &callgraph, const std::string &nodeName, const FunctionNames &callees,
                const FunctionNames &callers, const FunctionNames &overriddenBy,
                const FunctionNames &overriddenFunctions, const bool isVirtual, const bool doesOverride,
                const bool hasBody);

void readIPCG(std::string &filename, nlohmann::json &callgraph);

void writeIPCG(std::string &filename, nlohmann::json &callgraph);

#endif /* ifndef CGCOLLECTOR_JSONMANAGER_H */
