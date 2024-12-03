/**
 * File: SymbolRetriever.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#ifndef METACG_SYMBOLRETRIEVER_H
#define METACG_SYMBOLRETRIEVER_H

#include "LoggerUtil.h"
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace SymbolRetriever {

using SymbolTable = std::unordered_map<std::uintptr_t, std::string>;
using SymbolSet = std::unordered_set<std::string>;

using SymTableList = std::vector<std::pair<std::string, SymbolTable>>;
using SymbolSetList = std::vector<std::pair<std::string, SymbolSet>>;

struct MemMapEntry {
  std::string path;
  uintptr_t addrBegin;
  uint64_t offset;
};

struct MappedSymTable {
  SymbolTable table;
  MemMapEntry memMap;
};

using MappedSymTableMap = std::map<uintptr_t, MappedSymTable>;

/**
 * Loads symbols from the running process and maps their addresses into virtual memory.
 * @return
 */
MappedSymTableMap loadMappedSymTables(const std::string& execFile, bool printDebug = false);

SymbolTable loadSymbolTable(const std::string& object_file);
/**
 * Loads symbols from the executable and all shared library dependencies.
 * @param execFile
 * @return
 */
SymTableList loadAllSymTables(const std::string& execFile);

/**
 * Loads symbols from the executable and all shared library dependencies.
 * Does not save addresses, only symbol names.
 */
SymbolSetList loadSymbolSets(const std::string& execFile);

std::string findSymbol(uint64_t addrInProc, MappedSymTableMap&);

inline bool findSymbol(const SymbolSetList& symSets, const std::string& sym) {
  for (const auto& [_, symSet] : symSets) {
    if (symSet.find(sym) != symSet.end())
      return true;
  }
  return false;
}

std::string getExecPath();

inline uint64_t mapAddrToProc(uint64_t addrInLib, const MappedSymTable& table) {
  return table.memMap.addrBegin + addrInLib - table.memMap.offset;
}

inline uint64_t mapAddrToObj(uint64_t addrInProc, const MappedSymTable& table) {
  return addrInProc - table.memMap.addrBegin + table.memMap.offset;
}

inline std::unordered_set<std::string> getSymbolSet(const SymbolTable& table) {
  std::unordered_set<std::string> symSet;
  for (auto&& [key, val] : table) {
    symSet.insert(val);
  }
  return symSet;
}

}  // namespace SymbolRetriever
#endif  // METACG_SYMBOLRETRIEVER_H
