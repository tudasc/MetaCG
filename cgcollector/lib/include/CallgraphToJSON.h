#ifndef CGCOLLECTOR_CALLGRAPHTOJSON_H
#define CGCOLLECTOR_CALLGRAPHTOJSON_H

#include "JSONManager.h"
#include "MetaInformation.h"

#include "CallGraph.h"

void convertCallGraphToJSON(const CallGraph &cg, nlohmann::json &j);

void addMetaInformationToJSON(nlohmann::json &j, const std::string &metaInformationName,
                              const std::unordered_map<std::string, std::unique_ptr<MetaInformation>> &meta);

#endif /* ifndef CGCOLLECTOR_CALLGRAPHTOJSON_H */
