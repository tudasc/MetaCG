/**
 * File: VersionTwoMCGWriter.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "io/VersionTwoMCGWriter.h"
#include "MCGManager.h"
#include "io/VersionFourMCGWriter.h"
#include "metadata/OverrideMD.h"
#include <set>

using namespace metacg;

void metacg::io::VersionTwoMCGWriter::write(const metacg::Callgraph* cg, metacg::io::JsonSink& js) {
  // Check for nodes with duplicate function names - this is not supported by V2.
  if (cg->hasDuplicateNames()) {
    MCGLogger::logError(
        "There are multiple nodes with the same node - this is not allowed in format version 2."
        "Duplicate names will be disambiguated by adding a suffix to the function name."
        "Consider exporting using version 4 instead to retain the original names. ");
    // TOOD: Should this be considered fatal?
  }

  nlohmann::json j;
  attachMCGFormatHeader(j);

  io::JsonSink v4JsonSink;
  io::VersionFourMCGWriter v4Writer;
  v4Writer.setExportSorted(this->exportSorted);
  v4Writer.setUseNamesAsIds(true);
  v4Writer.write(cg, v4JsonSink);

  auto v4Json = v4JsonSink.getJson();
  auto& jsonCG = v4Json["_CG"];

  //  std::cout  << "Before: " << v4Json << "\n"; // FIXME: remove

  downgradeV4FormatToV2Format(jsonCG, exportSorted);

  j.at(fileInfo.formatInfo.cgFieldName) = std::move(jsonCG);
  //  std::cout  << "After: " << j << "\n"; // FIXME: Remove
  js.setJson(j);
}

void metacg::io::VersionTwoMCGWriter::downgradeV4FormatToV2Format(nlohmann::json& j, bool sortCallers) {
  // Iterate over all nodes
  for (auto& it : j.items()) {
    auto& jNode = it.value();

    // Function names are already used as keys, so we don't have to change anything here.
    jNode.erase("functionName");

    // Convert callees with edge metadata into a simple callee array. Callers are identified in a second pass, when all
    // nodes are present.
    auto jCalleesV2 = nlohmann::json::array();
    auto& jCallees = jNode["callees"];
    for (auto& jCallee : jCallees.items()) {
      jCalleesV2.push_back(jCallee.key());
      // Ignoring edge metadata, as not supported in V2
    }
    jNode.erase("callees");
    jNode["callers"] = nlohmann::json::array();
    jNode["callees"] = std::move(jCalleesV2);

    auto& jMeta = jNode["meta"];

    // If fileProperties metadata is already attached, put the origin entry there.
    // Otherwise, create the metadata if the origin is not null.
    if (jMeta.contains("fileProperties")) {
      // The expected behavior in V2 is to put an empty string if the origin is null
      if (jNode["origin"].is_null()) {
        jMeta["fileProperties"]["origin"] = "";
      } else {
        jMeta["fileProperties"]["origin"] = jNode["origin"];
      }
    } else if (!jNode["origin"].is_null()) {
      jMeta["fileProperties"] = nlohmann::json::object();
      jMeta["fileProperties"]["origin"] = jNode["origin"];
    }
    jNode.erase("origin");

    jNode["isVirtual"] = false;
    jNode["doesOverride"] = false;
    jNode["overrides"] = nlohmann::json::array();
    jNode["overriddenBy"] = nlohmann::json::array();
    if (jMeta.contains("overrideMD")) {
      jNode["isVirtual"] = true;
      auto& jOverrideMD = jMeta["overrideMD"];
      if (!jOverrideMD["overrides"].empty()) {
        jNode["doesOverride"] = true;
        jNode["overrides"] = jOverrideMD["overrides"];
      }
      jNode["overriddenBy"] = jOverrideMD["overriddenBy"];
      jMeta.erase("overrideMD");
      // If no other metadata exist, set meta field to null to maintain consistency.
      if (jMeta.empty()) {
        jMeta = nullptr;
      }
    }
  }

  // Iterate again to write callers
  for (auto& it : j.items()) {
    auto& jNode = it.value();
    for (auto& callee : jNode["callees"]) {
      j[callee]["callers"].push_back(it.key());
    }
  }

  // Finally, sort callers (if requested)
  if (sortCallers) {
    for (auto& it : j.items()) {
      auto& jNode = it.value();
      std::sort(jNode["callers"].begin(), jNode["callers"].end());
    }
  }
}
