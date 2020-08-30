#include "JSONManager.h"
#include "helper/common.h"

#include <clang/AST/Mangle.h>

#include <fstream>
#include <iostream>

void convertCallGraphToJSON(const CallGraph &cg, nlohmann::json &j) {
  for (auto it = ++cg.begin(); it != cg.end(); ++it) {
    if (auto f_decl = llvm::dyn_cast<clang::FunctionDecl>(it->getFirst())) {
      FunctionNames callees;

      // We can get multiple mangled names, as Ctor/Dtor can encode dofferent things
      auto mNames = getMangledName(f_decl);
      for (auto &it : *(it->getSecond())) {
        if (auto calleeDecl = llvm::dyn_cast<clang::FunctionDecl>(it->getDecl())) {
          auto calleeNames = getMangledName(calleeDecl);
          for (const auto &n : calleeNames) {
            // std::cout << mNames.front() << " -- " << n << std::endl;
          }
          callees.insert(std::begin(calleeNames), std::end(calleeNames));
        }
      }

      // method does not override any other method if list of overriden methods is empty
      bool doesOverride = !it->getSecond()->getOverriddenMethods().empty();
      // function is virtual if it is explicitly marked as virtual or if it does override any other method as only
      // virtual methods can be overridden
      bool isVirtual = f_decl->isVirtualAsWritten() || doesOverride;
      // overridden functions are collected during callgraph creation
      FunctionNames overriddenFunctions;
      for (auto f : it->getSecond()->getOverriddenMethods()) {
        auto overriddenNames = getMangledName(llvm::dyn_cast<clang::FunctionDecl>(f));
        overriddenFunctions.insert(std::begin(overriddenNames), std::end(overriddenNames));
      }
      FunctionNames overriddenBy;
      for (auto f : it->getSecond()->getOverriddenBy()) {
        auto overridingNames = getMangledName(llvm::dyn_cast<clang::FunctionDecl>(f));
        overriddenBy.insert(std::begin(overridingNames), std::end(overridingNames));
      }
      // which functions does call the current function
      FunctionNames callers;
      for (auto f : it->getSecond()->getParents()) {
        // this check has to be done because of the root element, which has a nullptr as the decl
        if (f) {
          // this check is not necessary because only function decls are add to the callgraph
          // if(auto fd = llvm::dyn_cast<clang::FunctionDecl>(f))
          auto parentNames = getMangledName(llvm::dyn_cast<clang::FunctionDecl>(f));
          callers.insert(std::begin(parentNames), std::end(parentNames));
        }
      }

      bool hasBody = f_decl->hasBody();

      // TODO names of fields
      for (const auto &n : mNames) {
        insertNode(j, n, callees, callers, overriddenBy, overriddenFunctions, isVirtual, doesOverride, hasBody);
      }
    }
  }
}

void addMetaInformationToJSON(nlohmann::json &j, const std::string &metaInformationName,
                              const std::unordered_map<std::string, std::unique_ptr<MetaInformation>> &meta) {
  for (auto &m : meta) {
    m.second->applyOnJSON(j, m.first, metaInformationName);
  }
}

void insertDefaultNode(nlohmann::json &callgraph, const std::string &nodeName) {
  const FunctionNames empty;
  insertNode(callgraph, nodeName, empty, empty, empty, empty, false, false, false);
}

void insertNode(nlohmann::json &callgraph, const std::string &nodeName, const FunctionNames &callees,
                const FunctionNames &callers, const FunctionNames &overriddenBy,
                const FunctionNames &overriddenFunctions, const bool isVirtual, const bool doesOverride,
                const bool hasBody) {
  callgraph[nodeName] = {{"callees", callees},
                         {"isVirtual", isVirtual},
                         {"doesOverride", doesOverride},
                         {"overriddenFunctions", overriddenFunctions},
                         {"overriddenBy", overriddenBy},
                         {"parents", callers},
                         {"hasBody", hasBody}};
}

void readIPCG(const std::string &filename, nlohmann::json &callgraph) {
  std::ifstream file(filename);
  file >> callgraph;
  file.close();
}

void writeIPCG(const std::string &filename, const nlohmann::json &callgraph) {
  std::ofstream file(filename);
  file << callgraph;
  file.close();
}
