
#include "JSONManager.h"
#include <string>
#include <set>
#include <queue>


FuncMapT::mapped_type &getOrInsert(std::string function, FuncMapT &functions) {
  if (functions.find(function) != functions.end()) {
    auto &fi = functions[function];
    return fi;
  } else {
    FunctionInfo fi;
    fi.functionName = function;
    functions.insert({function, fi});
    auto &rfi = functions[function];
    return rfi;
  }
}

nlohmann::json buildFromJSON(FuncMapT &functionMap, const std::string &filename) {
  using json = nlohmann::json;

  json j;
  readIPCG(filename, j);

  for (json::iterator it = j.begin(); it != j.end(); ++it) {
    auto &fi = getOrInsert(it.key(), functionMap);

    fi.functionName = it.key();
    int ns = it.value()["numStatements"].get<int>();
    bool hasBody = it.value()["hasBody"].get<bool>();
    fi.hasBody = hasBody;
    // ns == -1 means that there was no definition.
    if (ns > -1) {
      fi.numStatements = ns;
      fi.isVirtual = it.value()["isVirtual"].get<bool>();
      fi.doesOverride = it.value()["doesOverride"].get<bool>();
    }
    auto callees = it.value()["callees"].get<std::set<std::string>>();
    fi.callees.insert(callees.begin(), callees.end());

    auto ofs = it.value()["overriddenFunctions"].get<std::set<std::string>>();
    fi.overriddenFunctions.insert(ofs.begin(), ofs.end());

    auto overriddenBy = it.value()["overriddenBy"].get<std::set<std::string>>();
    fi.overriddenBy.insert(overriddenBy.begin(), overriddenBy.end());

    auto ps = it.value()["parents"].get<std::set<std::string>>();
    fi.parents.insert(ps.begin(), ps.end());
  }

  // Now the functions map holds all the information
  std::map<std::string, FunctionNames> potentialTargets;
  std::map<std::string, FunctionNames> overrides;

  for (auto [k, funcInfo] : functionMap) {
    if (!funcInfo.isVirtual) {
      // No virtual function, continue
      continue;
    }

    /*
     * The current function can: 1. override a function, or, 2. be overridden by a function
     *
     * (1) Add this function as potential target for any overridden function
     * (2) Add the overriding function as potential target for this function
     *
     */
    if (funcInfo.doesOverride) {
      for (const auto &overriddenFunction : funcInfo.overriddenFunctions) {
        // Adds this function as potential target to all overridden functions
        potentialTargets[overriddenFunction].insert(k);

        // In IPCG files, only the immediate overridden functions are stored currently.
        std::queue<std::string> workQ;
        std::set<std::string> visited;
        workQ.push(overriddenFunction);
        // Add this function as a potential target for all overridden functions
        while (!workQ.empty()) {
          const auto next = workQ.front();
          workQ.pop();

          const auto fi = functionMap[next];
          overrides[k].insert(next);
          visited.insert(next);

          potentialTargets[next].insert(k);
          for (const auto &om : fi.overriddenFunctions) {
            if (visited.find(om) == visited.end()) {
              workQ.push(om);
            }
          }
        }
      }
    }
  }

  // All potential parents is the set of keys in the potentialTargets set.
  // TODO Check if tests are correct for this case!
  // for (const auto [k, uoSet] : potentialTargets) {
  //  for (const auto name : uoSet) {
  //    auto &fi = functionMap[name];
  //    fi.parents.insert(k);
  //  }
  //}

  for (auto &[k, funcInfo] : functionMap) {
    funcInfo.overriddenBy.insert(potentialTargets[k].begin(), potentialTargets[k].end());
    funcInfo.overriddenFunctions.insert(overrides[k].begin(), overrides[k].end());
    // std::cout << "Targets for " << k << " are: ";
    // for (auto s : funcInfo.overriddenBy) {
    // std::cout << ", " << s;
    //}
    // std::cout << std::endl;
  }
  return j;
}


nlohmann::json buildFromJSONv2(FuncMapT &functionMap, const std::string &filename) {
  using json = nlohmann::json;

  std::cout << "Reading " << filename << std::endl;

  const auto readNumStmts = [] (auto &jsonElem, auto &funcItem) {
    auto metaInfo = jsonElem["meta"]["numStatements"];
      funcItem.numStatements = metaInfo;
  };
  const auto readFileOrigin = [] (auto &jsonElem, auto &funcItem) {
    auto foJson = jsonElem["meta"]["fileProperties"];
    if (foJson.is_null()) {
      exit(-1);
    }
    FilePropertyResult *fpMetaInfo = new FilePropertyResult();
    fpMetaInfo->fileOrigin = foJson["origin"].template get<std::string>();
    fpMetaInfo->isFromSystemInclude = foJson["systemInclude"].template get<bool>();
    funcItem.metaInfo["fileProperty"] = fpMetaInfo;
    };

  json jj;
  readIPCG(filename, jj);
  
  json j = jj["_CG"];

  if (j.is_null()) {
    std::cerr << "Encountered null element as top level CG" << std::endl;
  }

  for (json::iterator it = j.begin(); it != j.end(); ++it) {
    auto &fi = getOrInsert(it.key(), functionMap);

    fi.functionName = it.key();
    bool hasBody = it.value()["hasBody"].get<bool>();
    fi.hasBody = hasBody;

    bool isVirtual = it.value()["isVirtual"].get<bool>();
    fi.isVirtual = isVirtual;

    auto callees = it.value()["callees"].get<std::set<std::string>>();
    fi.callees.insert(callees.begin(), callees.end());

    auto ofs = it.value()["overrides"].get<std::set<std::string>>();
    fi.overriddenFunctions.insert(ofs.begin(), ofs.end());

    auto overriddenBy = it.value()["overriddenBy"].get<std::set<std::string>>();
    fi.overriddenBy.insert(overriddenBy.begin(), overriddenBy.end());

    auto ps = it.value()["callers"].get<std::set<std::string>>();
    fi.parents.insert(ps.begin(), ps.end());

    readNumStmts(it.value(), fi);
    readFileOrigin(it.value(), fi);
  }

  // Now the functions map holds all the information
  std::map<std::string, FunctionNames> potentialTargets;
  std::map<std::string, FunctionNames> overrides;

  for (auto [k, funcInfo] : functionMap) {
    if (!funcInfo.isVirtual) {
      // No virtual function, continue
      continue;
    }

    /*
     * The current function can: 1. override a function, or, 2. be overridden by a function
     *
     * (1) Add this function as potential target for any overridden function
     * (2) Add the overriding function as potential target for this function
     *
     */
    if (funcInfo.doesOverride) {
      for (const auto &overriddenFunction : funcInfo.overriddenFunctions) {
        // Adds this function as potential target to all overridden functions
        potentialTargets[overriddenFunction].insert(k);

        // In IPCG files, only the immediate overridden functions are stored currently.
        std::queue<std::string> workQ;
        std::set<std::string> visited;
        workQ.push(overriddenFunction);
        // Add this function as a potential target for all overridden functions
        while (!workQ.empty()) {
          const auto next = workQ.front();
          workQ.pop();

          const auto fi = functionMap[next];
          overrides[k].insert(next);
          visited.insert(next);

          potentialTargets[next].insert(k);
          for (const auto &om : fi.overriddenFunctions) {
            if (visited.find(om) == visited.end()) {
              workQ.push(om);
            }
          }
        }
      }
    }
  }

  for (auto &[k, funcInfo] : functionMap) {
    funcInfo.overriddenBy.insert(potentialTargets[k].begin(), potentialTargets[k].end());
    funcInfo.overriddenFunctions.insert(overrides[k].begin(), overrides[k].end());
  }
//  std::cout << "Filename: " << filename << "\n" << j << std::endl;
  return j;
}

