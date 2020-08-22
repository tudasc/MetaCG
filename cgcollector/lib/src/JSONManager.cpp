#include "JSONManager.h"
#include "helper/common.h"

#include <clang/AST/Mangle.h>

#include <unordered_set>

#include <iostream>

void convertCallGraphToJSON(const CallGraph &cg, nlohmann::json &j) {
  using FunctionNames = std::unordered_set<std::string>;

  for (auto it = ++cg.begin(); it != cg.end(); ++it) {
    if (auto f_decl = llvm::dyn_cast<clang::FunctionDecl>(it->getFirst())) {
      FunctionNames callees;
      auto mName = getMangledName(f_decl);
      for (auto &it : *(it->getSecond())) {
        if (auto f_decl = llvm::dyn_cast<clang::FunctionDecl>(it->getDecl())) {
          callees.insert(getMangledName(f_decl));
        }
      }

      // method does not override any other method if list of overriden methods is empty
      bool doesOverride = !it->getSecond()->getOverriddenMethods().empty();
      // function is virtual if it is explicitly marked as virtual or if it does override any other method as only
      // virtual methods can be overridden
      bool isVirtual = f_decl->isVirtualAsWritten() || doesOverride;
      // overridden functions are collected during callgraph creation
      FunctionNames overriddenFunctions;
      for (auto f : it->getSecond()->getOverriddenMethods())
        overriddenFunctions.insert(getMangledName(llvm::dyn_cast<clang::FunctionDecl>(f)));
      FunctionNames overriddenBy;
      for (auto f : it->getSecond()->getOverriddenBy())
        overriddenBy.insert(getMangledName(llvm::dyn_cast<clang::FunctionDecl>(f)));
      // which functions does call the current function
      FunctionNames callers;
      for (auto f : it->getSecond()->getParents()) {
        // this check has to be done because of the root element, which has a nullptr as the decl
        if (f)
          // this check is not necessary because only function decls are add to the callgraph
          // if(auto fd = llvm::dyn_cast<clang::FunctionDecl>(f))
          callers.insert(getMangledName(llvm::dyn_cast<clang::FunctionDecl>(f)));
      }

      bool hasBody = f_decl->hasBody();

      // TODO names of fields
      j[getMangledName(f_decl)] = {{"callees", callees},
                                   {"isVirtual", isVirtual},
                                   {"doesOverride", doesOverride},
                                   {"overriddenFunctions", overriddenFunctions},
                                   {"overriddenBy", overriddenBy},
                                   {"parents", callers},
                                   {"hasBody", hasBody}};
    }
  }
}

void addMetaInformationToJSON(nlohmann::json &j, const std::string &metaInformationName,
                              const std::unordered_map<std::string, std::unique_ptr<MetaInformation>> &meta) {
  for (auto &m : meta) {
    m.second->applyOnJSON(j, m.first, metaInformationName);
  }
}
