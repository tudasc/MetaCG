#include "nlohmann/json.hpp"

#include <fstream>
#include <queue>
#include <set>
#include <unordered_set>

#include <iostream>

struct FunctionInfo {
  FunctionInfo() : functionName("_UNDEF_"), isVirtual(false), doesOverride(false), numStatements(-1), hasBody(false) {}
  std::string functionName;
  bool isVirtual;
  bool doesOverride;
  int numStatements;
  bool hasBody;
  std::unordered_set<std::string> callees;
  std::unordered_set<std::string> parents;
  std::unordered_set<std::string> overriddenFunctions;
  std::unordered_set<std::string> overriddenBy;
};

typedef std::map<std::string, FunctionInfo> FuncMapT;

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
  {
    std::ifstream in(filename);
    if (!in.is_open()) {
      exit(-1);
    }
    in >> j;
  }

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
  std::map<std::string, std::unordered_set<std::string>> potentialTargets;
  std::map<std::string, std::unordered_set<std::string>> overrides;

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
      for (const auto overriddenFunction : funcInfo.overriddenFunctions) {
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
          for (const auto om : fi.overriddenFunctions) {
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

int main(int argc, char **argv) {
  if (argc < 3) {
    return -1;
  }

  std::vector<std::string> inputFiles;
  //inputFiles.reserve(argc - 2);
  for (int i = 2; i < argc; ++i) {
    inputFiles.emplace_back(std::string(argv[i]));
  }


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
  {
    std::cout << "Reading " << argv[1] << " as wholeCG file\n";
    std::ifstream file(argv[1]);
    file >> wholeCG;
  }

  std::cout << "Now starting merge of " << inputFiles.size() << " files." << std::endl;
  for (const auto &filename : inputFiles) {
    //std::cout << "[Info] Processing " << filename << std::endl;

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
          //std::cout << "WARNING: merge of " << it.key()
          //          << " has detected multiple bodies (equal number of statements would be good.)" << std::endl;
          // TODO check for equal values
          doMerge(c, v);
          // The num Statements should not differ
          if (c["numStatements"].get<int>() != v["numStatements"].get<int>()) {
            //std::cout << "[WARNING] Number of statements for function " << it.key() << " differ." << std::endl;
            //std::cout << "[WholeCG]: " << c["numStatements"].get<int>()
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

  std::ofstream file(argv[1]);
  file << wholeCG;

  return 0;
}
