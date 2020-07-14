#ifndef CGCOLLECTOR_JSONMANAGER_H
#define CGCOLLECTOR_JSONMANAGER_H

#include "MetaCollector.h"

#include "CallGraph.h"

#include "nlohmann/json.hpp"

void convertCallGraphToJSON(const CallGraph &cg, nlohmann::json &j);

void addMetaInformationToJSON(nlohmann::json &j, const std::string &metaInformationName,
                              const std::unordered_map<std::string, std::unique_ptr<MetaInformation>> &meta);

#endif /* ifndef CGCOLLECTOR_JSONMANAGER_H */
