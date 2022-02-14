#include "GlobalCallDepth.h"
#include <llvm/ADT/SmallSet.h>
#include <llvm/ADT/StringMap.h>
#include <queue>
#include <set>
#include <string>

int getLoopDepth(const std::string &entry, const llvm::StringMap<llvm::StringMap<int>> &callDepths,
                 const llvm::StringMap<int> &localLoopDepth, llvm::StringMap<int> &globalLoopDepth,
                 llvm::StringMap<bool> &visited) {
  // First mark ourselves as visited, to infinite loops
  visited.insert_or_assign(entry, true);
  int maxChildDepth = 0;
  const auto childrenit = callDepths.find(entry);
  assert(childrenit != callDepths.end());
  const auto children = childrenit->second;

  for (const auto &child : children) {
    if (visited.find(child.first()) == visited.end()) {
      maxChildDepth = std::max(maxChildDepth, child.second + getLoopDepth(child.first(), callDepths, localLoopDepth,
                                                                          globalLoopDepth, visited));
    }
  }
  // Remove mark
  visited.erase(entry);
  const int loopDepth = std::max(maxChildDepth, localLoopDepth.lookup(entry));
  auto globalLDIt = globalLoopDepth.find(entry);
  if (globalLDIt == globalLoopDepth.end()) {
    globalLoopDepth.insert({entry, loopDepth});
  } else {
    globalLDIt->second = std::max(globalLDIt->second, loopDepth);
  }
  return loopDepth;
}

void calculateGlobalCallDepth(nlohmann::json &j, bool useOnlyMainEntry) {
  // This only works for metadata version 2
  using MapT = llvm::StringMap<int>;
  llvm::StringMap<MapT> callDepths;
  llvm::StringMap<int> localLoopDepth;
  llvm::StringMap<int> globalLoopDepth;
  llvm::StringMap<bool> visited;

  auto &cg = j["_CG"];
  // Init the local call depths and the call graph
  for (const auto &[key, val] : cg.items()) {
    const auto &metaval = val["meta"];
    assert(metaval.count("loopDepth") == 1);
    bool inserted = localLoopDepth.insert({key, metaval["loopDepth"]}).second;
    assert(inserted);
    MapT called;
    assert(val.count("callees") == 1);
    for (const auto &c : val["callees"]) {
      called.insert({c.get<std::string>(), 0});
    }
    assert(metaval.count("loopCallDepth") == 1);
    for (const auto &[calledFunction, callDepth] : metaval["loopCallDepth"].items()) {
      called[calledFunction] = callDepth;
    }
    assert(callDepths.count(key) == 0);
    callDepths.insert_or_assign(key, called);
  }

  std::vector<std::string> entryPoints;

  // Find entry points
  if (!useOnlyMainEntry) {
    llvm::StringMap<bool> called;
    for (const auto &caller : callDepths) {
      called.insert({caller.first(), false});
      for (const auto &callee : caller.second) {
        if (callee.first() != caller.first()) {
          called.insert_or_assign(callee.first(), true);
        }
      }
    }
    for (const auto &f : called) {
      if (!f.second) {
        entryPoints.push_back(f.first());
      }
    }
  } else {
    if (callDepths.find("main") != callDepths.end()) {
      entryPoints.emplace_back("main");
    }
  }

  for (const auto &entry : entryPoints) {
    globalLoopDepth.insert_or_assign(entry, getLoopDepth(entry, callDepths, localLoopDepth, globalLoopDepth, visited));
  }

  for (auto &[key, val] : cg.items()) {
    const auto gcd = globalLoopDepth.lookup(key);
    val["meta"]["globalLoopDepth"] = std::max(gcd, val["meta"]["loopDepth"].get<int>());
  }
}
