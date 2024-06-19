/**
* File: VersionTwoMCGWriter.cpp
* License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
* https://github.com/tudasc/metacg/LICENSE.txt
*/

#include "io/VersionTwoMCGWriter.h"
#include "MCGManager.h"
#include "metadata/OverrideMD.h"
#include <set>

void metacg::io::VersionTwoMCGWriter::write(JsonSink &js) {
 nlohmann::json j;
 attachMCGFormatHeader(j);
 j.at(fileInfo.formatInfo.cgFieldName) = *mcgManager.getCallgraph();
 downgradeV3FormatToV2Format(j);
 js.setJson(j);
}

void metacg::io::VersionTwoMCGWriter::downgradeV3FormatToV2Format(nlohmann::json &j) {
 auto &cg = j.at("_CG");

 // rebuild caller callee maps
 std::unordered_map<size_t, std::string> idNameMap;
 std::unordered_map<size_t, std::vector<size_t>> sourceTargetEdgeMap;
 std::unordered_map<size_t, std::vector<size_t>> targetSourceEdgeMap;

 // rebuild caller callee maps
 for (auto &node : cg.at("nodes")) {
   idNameMap[node.at(0)] = node.at(1).at("functionName");
   node.at(1).erase("functionName");
 }

 for (auto &edge : cg.at("edges")) {
   sourceTargetEdgeMap[edge.at(0).at(0)].push_back(edge.at(0).at(1));
   targetSourceEdgeMap[edge.at(0).at(1)].push_back(edge.at(0).at(0));
 }
 cg.erase("edges");
 // move edges
 for (auto &node : cg.at("nodes")) {
   node.at(1)["callees"] = nlohmann::json::array();
   for (auto &callee : sourceTargetEdgeMap[node.at(0)]) {
     node.at(1).at("callees").push_back(idNameMap.at(callee));
   }
   node.at(1)["callers"] = nlohmann::json::array();
   for (auto &caller : targetSourceEdgeMap[node.at(0)]) {
     node.at(1).at("callers").push_back(idNameMap.at(caller));
   }
 }

 for (auto &node : cg.at("nodes")) {
   const auto &nodeName = idNameMap.at(node.at(0));
   j["CG"][nodeName] = std::move(node.at(1));

   // if we have attached origin information move it to Fileproperty data
   if(j["CG"][nodeName]["origin"] != "unknownOrigin"){
     j["CG"][nodeName].at("meta")["fileProperties"]={{"origin",j["CG"][nodeName]["origin"]},{"systemInclude", false}};
   }
   j["CG"][nodeName].erase("origin");

   // if we have override metadata, use it to generate override information
   if (!j["CG"][nodeName].at("meta").is_null() && j["CG"][nodeName].at("meta").contains("overrideMD")) {
     j["CG"][nodeName]["isVirtual"] = true;
     j["CG"][nodeName]["doesOverride"] = !j["CG"][nodeName].at("meta").at("overrideMD").at("overrides").empty();
     nlohmann::json overrideNames=nlohmann::json::array();
     for(const auto& n : j["CG"][nodeName].at("meta").at("overrideMD").at("overrides")){
       overrideNames.push_back(idNameMap.at(n));
     }
     j["CG"][nodeName]["overrides"] = overrideNames;
     nlohmann::json overriddenByNames =nlohmann::json::array();
     for(const auto& n : j["CG"][nodeName].at("meta").at("overrideMD").at("overriddenBy")){
       overriddenByNames.push_back(idNameMap.at(n));
     }
     j["CG"][nodeName]["overriddenBy"] =overriddenByNames;
     j["CG"][nodeName].at("meta").erase("overrideMD");
   } else {
     j["CG"][nodeName]["isVirtual"] = false;
     j["CG"][nodeName]["doesOverride"] = false;
     j["CG"][nodeName]["overrides"] = nlohmann::json::array();
     j["CG"][nodeName]["overriddenBy"] = nlohmann::json::array();
   }

   // for some reason, if we don't have metadata we don't generate an empty container, but null
   if (j["CG"][nodeName].at("meta").empty()) {
     j["CG"][nodeName].at("meta") = "null"_json;
   }
 }
 j.at("_CG") = std::move(j["CG"]);
 j.erase("CG");
}
