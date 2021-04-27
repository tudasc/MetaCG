#include "config.h"

#include "JSONManager.h"

#include <fstream>
#include <queue>
#include <set>

#include <iostream>

nlohmann::json mergeFileFormatTwo(std::string wholeCGFilename, std::vector<std::string> inputFiles) {
  const auto toSet = [&](auto &jsonObj, std::string id) {
    const auto &obj = jsonObj[id];
    std::set<std::string> target;
    for (const auto e : obj) {
      target.insert(e.template get<std::string>());
    }
    return target;
  };

  const auto doMerge = [&](auto &t, const auto &s) {
    auto &callees = s["callees"];
    std::set<std::string> tCallees{toSet(t, "callees")};
    for (auto c : callees) {
      tCallees.insert(c.template get<std::string>());
    }
    t["callees"] = tCallees;

    auto &overriddenFuncs = s["overrides"];
    std::set<std::string> tOverriddenFuncs{toSet(t, "overrides")};
    for (auto of : overriddenFuncs) {
      tOverriddenFuncs.insert(of.template get<std::string>());
    }

    t["isVirtual"] = s["isVirtual"];
    t["doesOverride"] = s["doesOverride"];
    t["hasBody"] = s["hasBody"];
  };

  FuncMapT functionInfoMap;
  nlohmann::json wholeCGFinal, wholeCG;
  std::cout << "Reading " << wholeCGFilename << " as wholeCG file\n";
  readIPCG(wholeCGFilename, wholeCGFinal);
  if (!wholeCGFinal.is_null()) {
    std::cerr << "Expecting empty json file to write Whole Program MetaCG to." << std::endl;
    exit(-1);
  }

  // {
  // "_Meta": {...}, <-- Gets attached by "attachFormatTwoHeader"
  // "_CG": { /** Gets constructed in the for loop **/ }
  // }
  attachFormatTwoHeader(wholeCGFinal);

  std::cout << "Now starting merge of " << inputFiles.size() << " files." << std::endl;
  for (const auto &filename : inputFiles) {
    // std::cout << "[Info] Processing " << filename << std::endl;

    auto current = buildFromJSONv2(functionInfoMap, filename);

    for (nlohmann::json::iterator it = current.begin(); it != current.end(); ++it) {
      if (wholeCG[it.key()].empty()) {
        wholeCG[it.key()] = it.value();
      } else {
        auto &c = wholeCG[it.key()];
        auto &v = it.value();
        // TODO multiple bodies possible, if the body is in header?
        // TODO separate merge of meta information
        if (v["hasBody"].get<bool>() && c["hasBody"].get<bool>()) {
          // std::cout << "WARNING: merge of " << it.key()
          //          << " has detected multiple bodies (equal number of statements would be good.)" << std::endl;
          // TODO check for equal values
          doMerge(c, v);
          // The num Statements should not differ
          if (c["meta"]["numStatements"].get<int>() != v["meta"]["numStatements"].get<int>()) {
            // std::cout << "[WARNING] Number of statements for function " << it.key() << " differ." << std::endl;
            // std::cout << "[WholeCG]: " << c["numStatements"].get<int>()
            //          << "\n[MergeCG]: " << v["numStatements"].get<int>() << std::endl;
            bool shouldAbort = false;
            if (shouldAbort) {
              abort();
            }
            c["meta"]["numStatements"] = c["meta"]["numStatements"].get<int>() + v["meta"]["numStatements"].get<int>();
          }
        } else if (v["hasBody"].get<bool>()) {
          doMerge(c, v);
          c["meta"]["numStatements"] = v["meta"]["numStatements"];
          c["meta"]["fileProperties"] = v["meta"]["fileProperties"];
        } else if (c["hasBody"].get<bool>()) {
          // callees, isVirtual, doesOverride and overriddenFunctions unchanged
          // hasBody unchanged
          // numStatements unchanged
        } else {
          // nothing special
        }

        // merge callers
        {
          auto v_parents = v["callers"].get<std::set<std::string>>();
          std::set<std::string> parents{c["callers"].get<std::set<std::string>>()};
          parents.insert(std::begin(v_parents), std::end(v_parents));
          c["callers"] = parents;
        }
      }
    }
  }

  for (auto it = wholeCG.begin(); it != wholeCG.end(); ++it) {
    auto curFunc = it.key();
    if (functionInfoMap.find(curFunc) != functionInfoMap.end()) {
      auto &jsonFunc = it.value();
      jsonFunc["overriddenBy"] = functionInfoMap[curFunc].overriddenBy;
      jsonFunc["overrides"] = functionInfoMap[curFunc].overriddenFunctions;
    }
  }

  wholeCGFinal["_CG"] = wholeCG;

  return wholeCGFinal;
}


nlohmann::json mergeFileFormatOne(std::string wholeCGFilename, std::vector<std::string> inputFiles) {
  const auto toSet = [&](auto &jsonObj, std::string id) {
    const auto &obj = jsonObj[id];
    std::set<std::string> target;
    for (const auto e : obj) {
      target.insert(e.template get<std::string>());
    }
    return target;
  };

  const auto doMerge = [&](auto &t, const auto &s) {
    auto &callees = s["callees"];
    std::set<std::string> tCallees{toSet(t, "callees")};
    for (auto c : callees) {
      tCallees.insert(c.template get<std::string>());
    }
    t["callees"] = tCallees;

    auto &overriddenFuncs = s["overriddenFunctions"];
    std::set<std::string> tOverriddenFuncs{toSet(t, "overriddenFunctions")};
    for (auto of : overriddenFuncs) {
      tOverriddenFuncs.insert(of.template get<std::string>());
    }

    t["isVirtual"] = s["isVirtual"];
    t["doesOverride"] = s["doesOverride"];
    t["hasBody"] = s["hasBody"];
  };

  FuncMapT functionInfoMap;
  nlohmann::json wholeCG;
  std::cout << "Reading " << wholeCGFilename << " as wholeCG file\n";
  readIPCG(wholeCGFilename, wholeCG);

  std::cout << "Now starting merge of " << inputFiles.size() << " files." << std::endl;
  for (const auto &filename : inputFiles) {
    // std::cout << "[Info] Processing " << filename << std::endl;

    auto current = buildFromJSON(functionInfoMap, filename);

    for (nlohmann::json::iterator it = current.begin(); it != current.end(); ++it) {
      if (wholeCG[it.key()].empty()) {
        wholeCG[it.key()] = it.value();
      } else {
        auto &c = wholeCG[it.key()];
        auto &v = it.value();
        // TODO multiple bodies possible, if the body is in header?
        // TODO separate merge of meta information
        if (v["hasBody"].get<bool>() && c["hasBody"].get<bool>()) {
          // std::cout << "WARNING: merge of " << it.key()
          //          << " has detected multiple bodies (equal number of statements would be good.)" << std::endl;
          // TODO check for equal values
          doMerge(c, v);
          // The num Statements should not differ
          if (c["numStatements"].get<int>() != v["numStatements"].get<int>()) {
            // std::cout << "[WARNING] Number of statements for function " << it.key() << " differ." << std::endl;
            // std::cout << "[WholeCG]: " << c["numStatements"].get<int>()
            //          << "\n[MergeCG]: " << v["numStatements"].get<int>() << std::endl;
            bool shouldAbort = false;
            if (shouldAbort) {
              abort();
            }
            c["numStatements"] = c["numStatements"].get<int>() + v["numStatements"].get<int>();
          }
        } else if (v["hasBody"].get<bool>()) {
          doMerge(c, v);
          c["numStatements"] = v["numStatements"];
        } else if (c["hasBody"].get<bool>()) {
          // callees, isVirtual, doesOverride and overriddenFunctions unchanged
          // hasBody unchanged
          // numStatements unchanged
        } else {
          // nothing special
        }

        // merge callers
        {
          auto v_parents = v["parents"].get<std::set<std::string>>();
          std::set<std::string> parents{c["parents"].get<std::set<std::string>>()};
          parents.insert(std::begin(v_parents), std::end(v_parents));
          c["parents"] = parents;
        }
      }
    }
  }

  for (auto it = wholeCG.begin(); it != wholeCG.end(); ++it) {
    auto curFunc = it.key();
    if (functionInfoMap.find(curFunc) != functionInfoMap.end()) {
      auto &jsonFunc = it.value();
      jsonFunc["overriddenBy"] = functionInfoMap[curFunc].overriddenBy;
      jsonFunc["overriddenFunctions"] = functionInfoMap[curFunc].overriddenFunctions;
    }
  }

  return wholeCG;
}

int main(int argc, char **argv) {
  if (argc < 3) {
    return -1;
  }

  std::cout << "Running MetaCG::CGMerge (version " << CGCollector_VERSION_MAJOR << '.' << CGCollector_VERSION_MINOR
            << ")\nGit revision: " << MetaCG_GIT_SHA << std::endl;

  std::vector<std::string> inputFiles;
  // inputFiles.reserve(argc - 2);
  for (int i = 2; i < argc; ++i) {
    inputFiles.emplace_back(std::string(argv[i]));
  }

    nlohmann::json j;
    readIPCG(inputFiles.front(), j);
    if (j.contains("_CG")) {
      auto wholeCG = mergeFileFormatTwo(argv[1], inputFiles);
      writeIPCG(argv[1], wholeCG);
    }
     else {
  auto wholeCG = mergeFileFormatOne(argv[1], inputFiles);
  writeIPCG(argv[1], wholeCG);
     }

  return 0;
}
