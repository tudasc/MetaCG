/**
* File: OverrideCollector.h
* License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
* https://github.com/tudasc/metacg/LICENSE.txt
*/

#ifndef CGCOLLECTOR2_OVERRIDECOLLECTOR_H
#define CGCOLLECTOR2_OVERRIDECOLLECTOR_H

#include "helper/Common.h"
#include "MetaDataFunctions.h"
#include "metadata/Internal/ASTNodeMetadata.h"
#include "Plugin.h"
#include "metadata/OverrideMD.h"

struct OverrideCollector : public Plugin {
  virtual void computeForGraph(const metacg::Callgraph* const cg) {
    for (auto& node : cg->getNodes()) {
      auto decl = node.second->get<ASTNodeMetadata>()->getFunctionDecl();

      if (auto MD = llvm::dyn_cast<clang::CXXMethodDecl>(decl); !MD || !MD->isVirtual()) {
        continue;
      }

      auto MD = llvm::cast<clang::CXXMethodDecl>(decl);

      // multiple inheritance causes multiple entries in overriden_methods
      // hierarchical overridden methods does not show up here
      for (auto om : MD->overridden_methods()) {
        // std::cout << "override " << om->getParent()->getNameAsString() << "::" << om->getNameAsString() << std::endl;

        for (auto& nodeName : getMangledName(om)) {
          // TODO once I rebased this onto devel with the new patch just use getNode
          if (!cg->hasNode(nodeName)) {
            // TODO: replace logging with spdlog
            std::cout << "Unknown node " << om->getNameAsString() << "\n";
            continue;
          }

          auto omNode = cg->getNode(nodeName);

          omNode->getOrCreateMD<OverrideMD>()->overriddenBy.push_back(node.first);
          node.second->getOrCreateMD<OverrideMD>()->overrides.push_back(omNode->getId());
        }
      }
    }
  }

  virtual std::string getPluginName() { return "OverrideCollector"; }

  virtual ~OverrideCollector() = default;
};

#endif  // CGCOLLECTOR2_OVERRIDECOLLECTOR_H
