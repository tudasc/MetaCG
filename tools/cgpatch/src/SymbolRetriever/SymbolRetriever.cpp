/**
 * File: SymbolRetriever.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "SymbolRetriever.h"

#include <cstring>
#include <elf.h>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <unistd.h>

namespace SymbolRetriever {

auto console = metacg::MCGLogger::instance().getConsole();
auto errConsole = metacg::MCGLogger::instance().getErrConsole();

struct RemoveEnvInScope {
  explicit RemoveEnvInScope(const char* varName) : varName(varName) {
    oldVal = getenv(varName);
    if (oldVal) {
      setenv(varName, "", true);
    }
  }

  ~RemoveEnvInScope() {
    if (oldVal) {
      setenv(varName, oldVal, true);
    }
  }

 private:
  const char* varName;
  const char* oldVal;
};

uintptr_t get_text_section_offset_from_library(const std::string& lib_path) {
  int fd = open(lib_path.c_str(), O_RDONLY);
  if (fd < 0) {
    perror("open");
    return 0;
  }

  Elf64_Ehdr ehdr;
  read(fd, &ehdr, sizeof(ehdr));

  lseek(fd, ehdr.e_phoff, SEEK_SET);

  Elf64_Phdr phdr;
  uintptr_t text_offset = 0;
  for (int i = 0; i < ehdr.e_phnum; i++) {
    read(fd, &phdr, sizeof(phdr));

    if (phdr.p_type == PT_LOAD && (phdr.p_flags & PF_X)) {
      // Found the executable segment in the shared library
      text_offset = phdr.p_vaddr - phdr.p_offset;
      break;
    }
  }

  close(fd);
  return text_offset;
}

std::string getExecPath() {
  RemoveEnvInScope removePreload("LD_PRELOAD");
  char filename[512] = {0};
  auto n = readlink("/proc/self/exe", filename, sizeof(filename) - 1);
  if (n > 0) {
    return filename;
  }
  return "";
}

std::vector<MemMapEntry> readMemoryMap() {
  RemoveEnvInScope removePreload("LD_PRELOAD");
  std::vector<MemMapEntry> entries;

  entries.reserve(1024);

  std::ifstream memoryMap("/proc/self/maps");
  if (!memory_map.is_open()) {
    console->info("Could not load memory map.");
    return entries;
  }

  std::string addrRange;
  std::string perms;
  uint64_t offset;
  std::string dev;
  uint64_t inode;
  std::string path;

  std::string line;
  while (std::getline(memory_map, line)) {
    std::istringstream lineStream(line);
    lineStream >> addrRange >> perms >> std::hex >> offset >> std::dec >> dev >> inode >> path;

    // Skip entries that are not "r-xp" or contain certain characters
    if (perms != "r-xp" || path.find("[") != std::string::npos) {
      continue;
    }

    uintptr_t addrBegin = std::stoul(addrRange.substr(0, addrRange.find('-')), nullptr, 16);
    // The offset reported by the memory map does not consider alignment.
    uint64_t textOffset = get_text_section_offset_from_library(path);
    entries.push_back({path, addrBegin, offset + textOffset});
  }

  entries.shrink_to_fit();
  memory_map.close();
  return entries;
}

std::vector<std::string> readSharedObjectDependencies(const std::string& exec_file) {
  RemoveEnvInScope removePreload("LD_PRELOAD");
  std::vector<std::string> dsoFiles;

  std::string command = "ldd " + exec_file;

  dsoFiles.reserve(64);

  char buffer[256];
  FILE* output = popen(command.c_str(), "r");
  if (!output) {
    console->info("Could not execute ldd.");
    return {};
  }

  std::string entry;
  std::string arrow;
  std::string entryPath;
  while (fgets(buffer, sizeof(buffer), output)) {
    std::istringstream lineStream(buffer);
    lineStream >> entry >> arrow;
    if (strcmp(arrow.c_str(), "=>") != 0) {
      continue;
    }
    lineStream >> entryPath;
    dsoFiles.push_back(entryPath);
  }
  pclose(output);
  return dsoFiles;
}

SymbolTable loadSymbolTable(const std::string& object_file) {
  // Need to disable LD_PRELOAD, otherwise this library will be loaded in popen call, if linked dynamically
  RemoveEnvInScope removePreload("LD_PRELOAD");

  std::string command = "nm --defined-only ";
  if (object_file.find(".so") != std::string::npos) {
    command += "-D ";
  }
  command += object_file;

  char buffer[256] = {0};
  FILE* output = popen(command.c_str(), "r");
  if (!output) {
    console->info("Unable to execute nm to resolve symbol names.");
    return {};
  }

  SymbolTable table;

  uintptr_t addr;
  std::string symType;
  std::string symName;

  while (fgets(buffer, sizeof(buffer), output)) {
    std::istringstream line(buffer);
    if (buffer[0] != '0') {
      continue;
    }
    line >> std::hex >> addr;
    line >> symType;
    line >> symName;
    table[addr] = symName;
  }
  pclose(output);

  if (table.empty()) {
    console->info("Unable to resolve symbol names for binary {}", object_file);
  }

  return table;
}

std::string getELFType(const std::string& object_file) {
  // Need to disable LD_PRELOAD, otherwise this library will be loaded in popen call, if linked dynamically
  RemoveEnvInScope removePreload("LD_PRELOAD");

  std::string command = "llvm-readelf -h ";
  command += object_file;
  command += " | grep Type";

  char buffer[256] = {0};
  FILE* output = popen(command.c_str(), "r");
  if (!output) {
    console->info("Unable to execute llvm-readelf.");
    return {};
  }

  std::string desc;
  std::string type;

  if (fgets(buffer, sizeof(buffer), output)) {
    std::istringstream line(buffer);
    line >> desc >> type;
  } else {
    console->info("Output buffer is empty.");
  }
  return type;
}

SymbolSetList loadSymbolSets(const std::string& execFile) {
  SymbolSetList symSets;

  // Load symbols from main executable
  auto execSyms = loadSymbolTable(execFile);
  if (execSyms.empty()) {
    return {};
  }
  auto dsoList = readSharedObjectDependencies(execFile);
  symSets.reserve(dsoList.size() + 1);
  symSets.emplace_back(execFile, getSymbolSet(execSyms));

  for (auto& entry : dsoList) {
    auto table = loadSymbolTable(entry);
    symSets.emplace_back(entry, getSymbolSet(table));
  }
  return symSets;
}

SymTableList loadAllSymTables(const std::string& execFile) {
  std::vector<std::pair<std::string, SymbolTable>> symTables;

  // Load symbols from main executable
  auto execSyms = loadSymbolTable(execFile);

  auto dsoList = readSharedObjectDependencies(execFile);
  symTables.reserve(dsoList.size() + 1);
  symTables.emplace_back(execFile, execSyms);

  for (auto& entry : dsoList) {
    auto table = loadSymbolTable(entry);
    symTables.emplace_back(entry, table);
  }
  return symTables;
}

MappedSymTableMap loadMappedSymTables(const std::string& execFile, bool printDebug) {
  MappedSymTableMap addrToSymTable;

  if (printDebug) {
    auto elfType = getELFType(execFile);
    errConsole->error("ELF type: {}", elfType);
  }

  // Load symbols from executable and shared libs
  auto memMap = readMemoryMap();
  for (auto& entry : memMap) {
    auto& filename = entry.path;
    auto table = loadSymbolTable(filename);
    if (table.empty()) {
      console->info("Could not load symbols from {}", filename);

      continue;
    }

    if (printDebug) {
      errConsole->error("Loaded {} symbols from {}.", table.size(), filename);
      errConsole->error("Starting address: 0x{}{}", std::hex, entry.addrBegin);
      errConsole->error(" > Offset: 0x{}{}", entry.offset, std::dec);
    }
    MappedSymTable mappedTable{std::move(table), entry};
    addrToSymTable[entry.addrBegin] = mappedTable;
  }
  return addrToSymTable;
}

std::string findSymbol(uint64_t addrInProc, MappedSymTableMap& mappedSymTables) {
  auto nextHighestIt = mappedSymTables.upper_bound(addrInProc);
  if (nextHighestIt == mappedSymTables.begin()) {
    return "";
  }
  nextHighestIt--;
  const auto& symbolTable = nextHighestIt->second;
  auto addrInObj = mapAddrToObj(addrInProc, symbolTable);
  auto it = symbolTable.table.find(addrInObj);
  if (it != symbolTable.table.end()) {
    return it->second;
  }
  return "";
}
}  // namespace SymbolRetriever
