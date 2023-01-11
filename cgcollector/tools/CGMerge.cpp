#include "config.h"

#include "AliasAnalysis.h"
#include "GlobalCallDepth.h"
#include "JSONManager.h"

#include <queue>
#include <set>

#include <iostream>

void mergeEquivalenceClasses(implementation::EquivClassContainer &File1Data, const nlohmann::json &File2,
                             nlohmann::json &wholeCG);
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
    std::cerr << "Expecting empty json file to write Whole Program metacg to." << std::endl;
    exit(-1);
  }

  // {
  // "_Meta": {...}, <-- Gets attached by "attachMCGFormatHeader"
  // "_CG": { /** Gets constructed in the for loop **/ }
  // "PointerEquivalenceData": {  /** Gets only constructed if any input contains it **/}
  // }
  attachFormatTwoHeader(wholeCGFinal);
  bool hasLoopInfo = false;
  bool hasAAInfo = false;

  implementation::EquivClassContainer File1Data;

  std::cout << "Now starting merge of " << inputFiles.size() << " files." << std::endl;
  for (const auto &filename : inputFiles) {
    // std::cout << "[Info] Processing " << filename << std::endl;

    nlohmann::json CompleteJson;
    auto current = buildFromJSONv2(functionInfoMap, filename, &CompleteJson);

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
          // assert(c["meta"]["numConditionalBranches"].get<int>() == v["meta"]["numConditionalBranches"].get<int>());
          // assert(c["meta"]["loopDepth"].get<int>() == v["meta"]["loopDepth"].get<int>());
        } else if (v["hasBody"].get<bool>()) {
          doMerge(c, v);
          c["meta"]["numStatements"] = v["meta"]["numStatements"];
          c["meta"]["fileProperties"] = v["meta"]["fileProperties"];
          // TODO JR, find some better way to check if merge is needed
          if (c["meta"].contains("loopDepth")) {
            hasLoopInfo = true;
            c["meta"]["numConditionalBranches"] = v["meta"]["numConditionalBranches"];
            c["meta"]["loopDepth"] = v["meta"]["loopDepth"];
            c["meta"]["numOperations"] = v["meta"]["numOperations"];
          }
        } else if (c["hasBody"].get<bool>()) {
          // callees, isVirtual, doesOverride and overriddenFunctions unchanged
          // hasBody unchanged
          // numStatements unchanged
        } else {
          // nothing special
        }

        // Merge the loop depths
        if (hasLoopInfo) {
          if (!c["meta"].contains("loopCallDepth") && v["meta"].contains("loopCallDepth")) {
            c["meta"]["loopCallDepth"] = v["meta"]["loopCallDepth"];
          } else if (c["meta"].contains("loopCallDepth") && v["meta"].contains("loopCallDepth")) {
            const auto vd = v["meta"]["loopCallDepth"];
            auto &vc = c["meta"]["loopCallDepth"];
            for (auto &[calledFunction, callDepths] : vc.items()) {
              const auto other = vd.find(calledFunction);
              if (other != vd.end()) {
                const int otherv = *other;
                if (otherv > callDepths) {
                  callDepths = otherv;
                }
              }
            }
            for (const auto &[calledFunction, callDepths] : vd.items()) {
              if (!vc.contains(calledFunction)) {
                vc[calledFunction] = callDepths;
              }
            }
          }
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

    // TODO: Maybe bump version to 3 ?
    if (CompleteJson.contains("PointerEquivalenceData")) {
      hasAAInfo = true;
      mergeEquivalenceClasses(File1Data, CompleteJson["PointerEquivalenceData"], wholeCG);
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
  if (hasAAInfo) {
    wholeCGFinal["PointerEquivalenceData"] = File1Data;
  }
  if (hasLoopInfo) {
    if (hasAAInfo) {
      // FIXME The global loop depth currently does not work together,
      //  i am not exactly sure why this is the case, but i assume it is caused by the global loop depth only using the
      //  call information from the original call graph construction, which can be different from the ones calculated by
      //  the AA
      llvm::errs() << "Global loop depth currently does not work together with the Alias Analysis\n";
    } else {
      calculateGlobalCallDepth(wholeCGFinal, true);
    }
  }

  return wholeCGFinal;
}

void addCallToCallGraph(const implementation::EquivClassContainer &Data, const StringType &CallingFunctionName,
                        const StringType &CalledFunctionName, nlohmann::json &Json) {
  const auto ItCalling = Data.FunctionInfoMap.find(CallingFunctionName);
  assert(ItCalling != Data.FunctionInfoMap.end());
  const auto CallingFunctionNamesMangled = ItCalling->second.MangledNames;
  const auto ItCalled = Data.FunctionInfoMap.find(CalledFunctionName);
  assert(ItCalled != Data.FunctionInfoMap.end());
  const auto CalledFunctionNamesMangled = ItCalled->second.MangledNames;
  for (const auto &CallingFunctionNameMangled : CallingFunctionNamesMangled) {
    auto Callees = Json.at(CallingFunctionNameMangled).at("callees").get<std::set<std::string>>();
    auto Inserted = false;
    for (const auto &CalledFunctionNameMangled : CalledFunctionNamesMangled) {
      Inserted |= Callees.insert(CalledFunctionNameMangled).second;
    }
#ifdef DEBUG_TEST_AA
    if (Inserted) {
      llvm::errs() << "Found Cross TU call: " << CallingFunctionName << " calls " << CalledFunctionName << "\n";
    }
#endif
    Json[CallingFunctionNameMangled]["callees"] = Callees;

    for (const auto &CalledFunctionNameMangled : CalledFunctionNamesMangled) {
      const auto It = Json.find(CalledFunctionNameMangled);
      if (It != Json.end()) {
        auto Parents = It->at("callers").get<std::set<std::string>>();
        Parents.insert(CallingFunctionNameMangled);
        Json[CalledFunctionNameMangled]["callers"] = Parents;
      } else {
        llvm::errs() << "Warning: Found call to " << CalledFunctionNameMangled << " ( " << CalledFunctionName
                     << " ) but it is not in the callgraph!\n";
      }
    }
  }
}

void mergeFunctionCall(implementation::EquivClassContainer &Data, const StringType &CalledObj,
                       implementation::CallInfoConstIterType CE, nlohmann::json &Json) {
  auto Ret = implementation::mergeFunctionCall(Data, CalledObj, CE);
  if (Ret) {
    addCallToCallGraph(Data, Ret->second.first, Ret->second.second, Json);
    for (const auto &ToMerge : Ret->first) {
      mergeFunctionCall(Data, ToMerge.first, ToMerge.second, Json);
    }
  }
}

/**
 *
 * @param File1Data Equivalence Information Container of file 1 (also output)
 * @param File2 Equivalence Information Container of file 2
 * @param wholeCG Output to the _CG json. Only used for adding edges to the callgraph
 */
void mergeEquivalenceClasses(implementation::EquivClassContainer &File1Data, const nlohmann::json &File2,
                             nlohmann::json &wholeCG) {
  const implementation::EquivClassContainer File2Data = File2;

  // FunctionMap
  for (const auto &Function : File2Data.FunctionMap) {
    File1Data.FunctionMap.emplace(Function);
  }

  VectorType<std::pair<StringType, StringType>> ParamsToMerge;

  // FunctionInfoMap
  for (const auto &FunctionInfo : File2Data.FunctionInfoMap) {
    const auto Iter = File1Data.FunctionInfoMap.find(FunctionInfo.first);
    if (Iter == File1Data.FunctionInfoMap.end()) {
      File1Data.FunctionInfoMap.emplace(FunctionInfo);
    } else {
      assert(Iter->second.Parameters.size() == FunctionInfo.second.Parameters.size());
      assert(Iter->second.IsVariadic == FunctionInfo.second.IsVariadic);
      assert(Iter->second.MangledNames == FunctionInfo.second.MangledNames);
      for (std::size_t I = 0; I < Iter->second.Parameters.size(); I++) {
        const auto &Name1 = Iter->second.Parameters[I];
        const auto &Name2 = FunctionInfo.second.Parameters[I];
        if (Name1 != Name2) {
          ParamsToMerge.emplace_back(Name1, Name2);
        }
      }
      // Can be the case if we only have a declaration
      if (Iter->second.ReferencedInReturnStmts.empty()) {
        Iter->second.ReferencedInReturnStmts = FunctionInfo.second.ReferencedInReturnStmts;
      }
    }
  }

  // CallInfoMap
  for (const auto &CallInfo : File2Data.CallInfoMap) {
    File1Data.CallInfoMap.emplace(CallInfo);
  }
  // ReferencedInCalls
  File1Data.InitReferencedInCalls();

  // CallExprParentMap
  for (const auto &CallExpr : File2Data.CallExprParentMap) {
    File1Data.CallExprParentMap.emplace(CallExpr);
  }

  VectorType<std::pair<StringType, implementation::CallInfoConstIterType>> FunctionsToMerge;
  VectorType<std::pair<StringType, StringType>> ObjectsToMerge;
  // EquivClasses
  // FindMap
  for (const auto &Equiv : File2Data.EquivClasses) {
    /*  Here we need to handle three different cases:
     *  1. The complete equiv class only exists on the right side:
     *  copy to the left
     *  2. All elements are in a single equiv class on the left side:
     *  merge prefixes ( copy new elements from the right side to them)
     *  3. The elements are in different classes on the left side:
     *  Merge on the left side and copy new elements from the right side to them
     */
    if (Equiv.Objects.size() == 1) {
      auto It = File1Data.FindMap.find(Equiv.Objects[0]);
      if (It != File1Data.FindMap.end()) {
        // Case 2
        implementation::GetPrefixesToMerge(*It->second, Equiv.Prefixes, ObjectsToMerge);
        std::sort(It->second->Prefixes.begin(), It->second->Prefixes.end());
        It->second->Prefixes.erase(std::unique(It->second->Prefixes.begin(), It->second->Prefixes.end()),
                                   It->second->Prefixes.end());
      } else {
        // Case 1
        File1Data.EquivClasses.emplace_front(Equiv);
        File1Data.FindMap.emplace(Equiv.Objects[0], File1Data.EquivClasses.begin());
      }
    } else {
      // We have at least two elements
      bool AllNew = true;
      auto ItFirst = File1Data.FindMap.find(Equiv.Objects[0]);
      auto ItFirstInner = File1Data.EquivClasses.end();
      if (ItFirst != File1Data.FindMap.end()) {
        AllNew = false;
        ItFirstInner = ItFirst->second;
      }
      for (int I = 1; I < Equiv.Objects.size(); I++) {
        auto ItSecond = File1Data.FindMap.find(Equiv.Objects[I]);
        if (AllNew) {
          AllNew &= (ItSecond == File1Data.FindMap.end());
          if (!AllNew) {
            ItFirstInner = ItSecond->second;
          }
        } else {
          if (ItSecond != File1Data.FindMap.end() && ItSecond->second != ItFirstInner) {
            // Merge the classes pointed to by ItSecond and ItFirst
            // Case 3
            assert(ItFirstInner != File1Data.EquivClasses.end());

            const auto Tmp = implementation::merge(File1Data, ItFirstInner, ItSecond->second);
            ObjectsToMerge.insert(ObjectsToMerge.end(), Tmp.ObjectsToMerge.begin(), Tmp.ObjectsToMerge.end());
            ItFirstInner = Tmp.Iter->second;
          } else {
            // Nothing to do, the element is new or in the same equivalence class
          }
        }
      }
      if (AllNew) {
        // Case 1:
        File1Data.EquivClasses.emplace_front(Equiv);
        for (const auto &E : Equiv.Objects) {
          File1Data.FindMap.emplace(E, File1Data.EquivClasses.begin());
        }
      } else {
        // Case 2, 3:
        assert(ItFirstInner != File1Data.EquivClasses.end());
        ItFirstInner->Objects.insert(ItFirstInner->Objects.end(), Equiv.Objects.begin(), Equiv.Objects.end());
        std::sort(ItFirstInner->Objects.begin(), ItFirstInner->Objects.end());
        ItFirstInner->Objects.erase(std::unique(ItFirstInner->Objects.begin(), ItFirstInner->Objects.end()),
                                    ItFirstInner->Objects.end());

        implementation::GetPrefixesToMerge(*ItFirstInner, Equiv.Prefixes, ObjectsToMerge);
        std::sort(ItFirstInner->Prefixes.begin(), ItFirstInner->Prefixes.end());
        ItFirstInner->Prefixes.erase(std::unique(ItFirstInner->Prefixes.begin(), ItFirstInner->Prefixes.end()),
                                     ItFirstInner->Prefixes.end());

        for (const auto &E : Equiv.Objects) {
          File1Data.FindMap.emplace(E, ItFirstInner);
        }
      }
    }
  }

  for (const auto &Object : ObjectsToMerge) {
    const auto It1 = File1Data.FindMap.find(Object.first);
    const auto It2 = File1Data.FindMap.find(Object.second);
    if (It1->second != It2->second) {
      const auto Tmp = implementation::mergeRecurisve(File1Data, It1->second, It2->second);
    }
  }

  for (const auto &Params : ParamsToMerge) {
    const auto Iter1 = File1Data.FindMap.find(Params.first);
    const auto Iter2 = File1Data.FindMap.find(Params.second);
    if (Iter1->second != Iter2->second) {
      const auto Tmp = implementation::mergeRecurisve(File1Data, Iter1->second, Iter2->second);
      FunctionsToMerge.insert(FunctionsToMerge.end(), Tmp.first.begin(), Tmp.first.end());
    }
  }

  for (const auto &E : File1Data.EquivClasses) {
    implementation::GetFunctionsToMerge(File1Data, FunctionsToMerge, E);
  }

  for (const auto &ToMerge : FunctionsToMerge) {
    mergeFunctionCall(File1Data, ToMerge.first, ToMerge.second, wholeCG);
  }
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

  std::cout << "Running metacg::CGMerge (version " << CGCollector_VERSION_MAJOR << '.' << CGCollector_VERSION_MINOR
            << ")\nGit revision: " << MetaCG_GIT_SHA << std::endl;

  std::vector<std::string> inputFiles;
  // inputFiles.reserve(argc - 2);
  for (int i = 2; i < argc; ++i) {
    inputFiles.emplace_back(std::string(argv[i]));
  }

  nlohmann::json j;
  bool foundValidFile{false};
  bool useFileFormatTwo{false};

  for (auto &inFile : inputFiles) {
    readIPCG(inFile, j);
    if (!j.is_null()) {
      foundValidFile = true;
      if (j.contains("_CG")) {
        useFileFormatTwo = true;
      }
      break;
    }
  }

  if (!foundValidFile) {
    std::cerr << "[Error] All input files are NULL" << std::endl;
    exit(EXIT_FAILURE);
  }
  j.clear();
  if (useFileFormatTwo) {
    auto wholeCG = mergeFileFormatTwo(argv[1], inputFiles);
    writeIPCG(argv[1], wholeCG);
  } else {
    auto wholeCG = mergeFileFormatOne(argv[1], inputFiles);
    writeIPCG(argv[1], wholeCG);
  }

  std::cout << "Done merging" << std::endl;

  return 0;
}
