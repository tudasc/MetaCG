/**
 * File: CallGraphGenerator.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */
#include "cage/generator/CallgraphGenerator.h"

#include "Callgraph.h"

#ifdef HAVE_METAVIRT
#include "metavirt/VirtCall.h"
#endif

#include "llvm/Analysis/CallGraph.h"
#include "llvm/Demangle/Demangle.h"
#include "llvm/IR/Function.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/IPO/WholeProgramDevirt.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"

#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/InstVisitor.h"

#include <cxxabi.h>

using namespace llvm;

namespace cage {

struct CallBaseVisitor : public llvm::InstVisitor<CallBaseVisitor> {
  CallBaseVisitor(llvm::CallGraph* lcg) : lcg(lcg), mcg(std::make_unique<metacg::Callgraph>()) {
    const Module& m = lcg->getModule();
    llvm::DebugInfoFinder dbg_finder{};
    dbg_finder.processModule(m);
    metaDataAvail = dbg_finder.subprogram_count() != 0;

    for (const auto& i : dbg_finder.subprograms()) {
      if (auto f = m.getFunction(i->getName())) {
        // We found the function with its normal name (C-Style function)
        functionInfoMap[f] = i;
      } else if (auto f = m.getFunction(i->getLinkageName())) {
        // We found the function via the linkage name (mangled name / C++ function)
        functionInfoMap[f] = i;
      } else {
        // assert(false);
      }
    }

    for (const auto& func : m.getFunctionList()) {
      signatureFunctionMap[func.getFunctionType()].push_back(&func.getFunction());
    }
  }

  void visitCallBase(llvm::CallBase& I) {
    if (I.getCalledFunction() != nullptr && I.getCalledFunction()->isIntrinsic())
      return;
    auto* currentFunction = I.getParent()->getParent();
    auto& currentNode = mcg->getSingleNode(currentFunction->getName().str());
    if (metaDataAvail) {
      size_t numAddedCalls = addVirtualCalltargets(I, currentNode);
      // This function pointer was a virtual call base, so we do not need to run the overapproximation
      if (numAddedCalls != 0)
        return;
    }

    if (I.getCalledFunction() == nullptr) {
      // Was function pointer, where we can not get the called function
      const auto& possibleFuncs = signatureFunctionMap[I.getFunctionType()];
      for (const auto& func : possibleFuncs) {
        metacg::CgNode& childNode = (metaDataAvail && functionInfoMap[func] != nullptr
                                         ? mcg->getOrInsertNode(functionInfoMap[func]->getLinkageName().str(),
                                                                functionInfoMap[func]->getFilename().str())
                                         : mcg->getOrInsertNode(func->getName().str()));
        mcg->addEdge(currentNode, childNode);
      }
    }
  }

  void visitFunction(llvm::Function& F) {
    if (F.isIntrinsic())
      return;
    const std::string& funcName = F.getName().str();
    metacg::CgNode& currentNode = (metaDataAvail && functionInfoMap[&F] != nullptr
                                       ? mcg->getOrInsertNode(funcName, functionInfoMap[&F]->getFilename().str())
                                       : mcg->getOrInsertNode(funcName));

    auto* lcgNode = lcg->operator[](&F);
    for (auto [key, elem] : *lcgNode) {
      if (!key.has_value())
        continue;
      if (elem->getFunction() == nullptr)
        continue;
      if (elem->getFunction()->isIntrinsic())
        continue;
      const Function* childFunc = elem->getFunction();
      assert(childFunc->hasName());
      metacg::CgNode& childNode =
          (metaDataAvail && functionInfoMap[childFunc] != nullptr
               ? mcg->getOrInsertNode(childFunc->getName().str(), functionInfoMap[childFunc]->getFilename().str())
               : mcg->getOrInsertNode(childFunc->getName().str()));
      mcg->addEdge(currentNode, childNode);
    }
  }

  std::unique_ptr<metacg::Callgraph> takeResult() { return std::move(mcg); }

 private:
  size_t addVirtualCalltargets(CallBase& I, const metacg::CgNode& currentNode) const {
    // TODO: Improve this design if we want to support multiple virtual call resolution mechanisms, e.g. with
    //       template policy parameter.
#ifdef HAVE_METAVIRT
    auto vcallData = metavirt::vcall_data_for(&I);
    if (!vcallData.has_value())
      return 0;
    if (vcallData.value().call_targets.empty())
      return 0;

    outs() << metavirt::fn_names_and_origins(vcallData.value()).size() << "\n";
    for (const auto& dataPoints : metavirt::fn_names_and_origins(vcallData.value())) {
      auto& childNode = mcg->getOrInsertNode(dataPoints.name.str(), dataPoints.origin.str());
      mcg->addEdge(currentNode, childNode);
      assert(childNode.getOrigin() == dataPoints.origin);
    }
    return metavirt::fn_names_and_origins(vcallData.value()).size();
#else
    return 0;
#endif
  }

  std::unique_ptr<metacg::Callgraph> mcg;
  llvm::CallGraph* lcg;
  bool metaDataAvail = false;
  std::unordered_map<const Function*, const llvm::DISubprogram*> functionInfoMap;
  std::unordered_map<llvm::FunctionType*, std::vector<const Function*>> signatureFunctionMap;
};

bool Generator::run(Module& M, ModuleAnalysisManager* MA) {
  auto& cgResult = MA->getResult<CallGraphAnalysis>(M);
  auto cbv = CallBaseVisitor(&cgResult);
  cbv.visit(M);

  // Take resulting metacg call graph
  auto mcg = cbv.takeResult();

  // Run registered consumers
  for (auto& consumer : consumers) {
    consumer->consumeCallGraph(*mcg);
  }

  return false;
}
}  // namespace cage
