#ifndef CGCOLLECTOR_CALLGRAPHTOJSON_H
#define CGCOLLECTOR_CALLGRAPHTOJSON_H

#include "JSONManager.h"
#include "MetaInformation.h"

#include "CallGraph.h"

void convertCallGraphToJSON(const CallGraph &cg, nlohmann::json &j, const int version);

void addMetaInformationToJSON(nlohmann::json &j, const std::string &metaInformationName,
                              const std::map<std::string, std::unique_ptr<MetaInformation>> &meta,
                              int mcgFormatVersion);

#endif /* ifndef CGCOLLECTOR_CALLGRAPHTOJSON_H */
