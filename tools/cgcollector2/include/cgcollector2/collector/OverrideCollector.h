/**
* File: OverrideCollector.h
* License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
* https://github.com/tudasc/metacg/LICENSE.txt
*/

#ifndef CGCOLLECTOR2_OVERRIDECOLLECTOR_H
#define CGCOLLECTOR2_OVERRIDECOLLECTOR_H

#include "metacg/metadata/OverrideMD.h"
#include "cgcollector2/interface/CGC2Plugin.h"
#include "cgcollector2/metadata/Internal/ASTNodeMetadata.h"

#include <clang/AST/Mangle.h>

struct OverrideCollector : public cgcollector2::Plugin {
  /**
 * Returns mangled names for all named decls, including Ctor/Dtor.
 */
  static std::vector<std::string> getMangledName(clang::NamedDecl const* const nd) {
    if (!nd) {
      llvm::errs() << "NamedDecl was nullptr\n";
      assert(nd && "NamedDecl and MangleContext must not be nullptr");
      return {"__NO_NAME__"};
    }
    clang::ASTNameGenerator NG(nd->getASTContext());

    if (llvm::isa<clang::CXXRecordDecl>(nd) || llvm::isa<clang::CXXMethodDecl>(nd) ||
        llvm::isa<clang::ObjCInterfaceDecl>(nd) || llvm::isa<clang::ObjCImplementationDecl>(nd)) {
      return NG.getAllManglings(nd);
        }
    return {NG.getName(nd)};
  }

  virtual void computeForGraph(metacg::Callgraph* const cg) {
    for (auto& node : cg->getNodes()) {
      auto decl = node->get<ASTNodeMetadata>()->getFunctionDecl();

      if (auto MD = llvm::dyn_cast<clang::CXXMethodDecl>(decl); !MD || !MD->isVirtual()) {
        continue;
      }

      auto MD = llvm::cast<clang::CXXMethodDecl>(decl);

      // multiple inheritance causes multiple entries in overriden_methods
      // hierarchical overridden methods does not show up here
      for (auto om : MD->overridden_methods()) {
        for (auto& nodeName : getMangledName(om)) {
          const auto& nodesWithMatchingName=cg->getNodes(nodeName);
          assert(nodesWithMatchingName.size()==1 && "We currently only validated this for collision free names");
          const auto omNode = cg->getNode(nodesWithMatchingName[0]);
          if (omNode == nullptr) {
            metacg::MCGLogger::logWarn("Node {} tries to override unknown node {} ",nodeName, om->getNameAsString());
            continue;
          }
          omNode->getOrCreate<metacg::OverrideMD>().overriddenBy.push_back(node->getId());
          node->getOrCreate<metacg::OverrideMD>().overrides.push_back(omNode->getId());
        }
      }
    }
  }

  std::string getPluginName() const final { return "OverrideCollector"; }

  virtual ~OverrideCollector() = default;
};

#endif  // CGCOLLECTOR2_OVERRIDECOLLECTOR_H
