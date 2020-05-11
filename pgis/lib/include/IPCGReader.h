#ifndef CUBECALLGRAPHTOOL_IPCG_READER_H
#define CUBECALLGRAPHTOOL_IPCG_READER_H

#include "CallgraphManager.h"

#include <string>
#include <unordered_map>
#include <unordered_set>

namespace IPCGAnal {

struct FunctionInfo {
  FunctionInfo() : functionName("_UNDEF_"), isVirtual(false), doesOverride(false), numStatements(-1) {}
  std::string functionName;
  bool isVirtual;
  bool doesOverride;
  int numStatements;
  std::unordered_set<std::string> callees;
  std::unordered_set<std::string> parents;
  std::unordered_set<std::string> overriddenFunctions;
};

typedef std::unordered_map<std::string, FunctionInfo> FuncMapT;

void build(CallgraphManager &cg, std::string filepath, Config *c);

int addRuntimeDispatchCallsFromCubexProfile(CallgraphManager &ipcg, CallgraphManager &cubecg);

FuncMapT::mapped_type &getOrInsert(std::string key, FuncMapT &functions);
void buildFromJSON(CallgraphManager &cgm, std::string filename, Config *c);
}  // namespace IPCGAnal

#endif
