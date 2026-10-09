/**
 * File: CallGraphGenerator.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */
#include "cage/generator/CallgraphGenerator.h"

#include "metacg/Callgraph.h"
#include "metacg/LoggerUtil.h"
#include "metacg/io/IdMapping.h"
#include "metacg/io/NameMapping.h"
#include "metacg/metadata/CallTypeMD.h"
#include "metacg/metadata/MetaData.h"
#include "metacg/metadata/MetadataMixin.h"
#include "metacg/metadata/OverrideMD.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/raw_ostream.h"
#include <exception>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

#ifdef HAVE_METAVIRT
#include "metavirt/VirtCall.h"
#endif

#include "llvm/Analysis/CallGraph.h"
#include "llvm/IR/Function.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Transforms/IPO/WholeProgramDevirt.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"

#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/InstVisitor.h"

#include <cxxabi.h>

using namespace llvm;

namespace cage {

namespace {

void collectAnnotations(std::unordered_map<std::string, std::vector<std::string>>& overrides, llvm::Module& M) {
  auto* GlobalAnnots = M.getNamedGlobal("llvm.global.annotations");
  if (!GlobalAnnots) {
    return;
  }

  auto* ArrayInit = llvm::dyn_cast<llvm::ConstantArray>(GlobalAnnots->getInitializer());

  if (!ArrayInit) {
    return;
  }

  for (unsigned int i = 0; i < ArrayInit->getNumOperands(); ++i) {
    auto* StructInit = llvm::dyn_cast<llvm::ConstantStruct>(ArrayInit->getOperand(i));

    if (!StructInit) {
      continue;
    }

    llvm::Value* AnnotatedFunc = StructInit->getOperand(0)->stripPointerCasts();
    llvm::Value* AnnotNameGlobal = StructInit->getOperand(1)->stripPointerCasts();
    llvm::Value* ArgsGlobal = StructInit->getOperand(4)->stripPointerCasts();

    llvm::StringRef AnnotatedFuncName = AnnotatedFunc->getName();
    llvm::StringRef annotStr = "";

    // Get name of overridden function
    if (auto* GV = llvm::dyn_cast<llvm::GlobalVariable>(AnnotNameGlobal)) {
      if (auto* CDA = llvm::dyn_cast<llvm::ConstantDataArray>(GV->getInitializer())) {
        annotStr = CDA->getAsString().drop_back(1);
      }
    }

    // Extract payload from annotation args and check if annotation belongs to overridePlugin
    bool isOverridePlugin = false;
    if (ArgsGlobal) {
      if (auto* GV = llvm::dyn_cast<llvm::GlobalVariable>(ArgsGlobal)) {
        if (auto* CS = llvm::dyn_cast<llvm::ConstantStruct>(GV->getInitializer())) {
          llvm::Value* PtrExpr = CS->getOperand(0);

          if (auto* CE = llvm::dyn_cast<llvm::ConstantExpr>(PtrExpr)) {
            if (CE->getOpcode() == llvm::Instruction::PtrToInt) {
              PtrExpr = CE->getOperand(0);  // Removes PtrToInt
            }
          }

          PtrExpr = PtrExpr->stripPointerCasts();

          if (auto* PayloadGV = llvm::dyn_cast<llvm::GlobalVariable>(PtrExpr)) {
            if (auto* CDA = llvm::dyn_cast<llvm::ConstantDataArray>(PayloadGV->getInitializer())) {
              llvm::StringRef payload = CDA->getAsString();

              if (payload.starts_with("overridePlugin")) {
                isOverridePlugin = true;
                overrides[AnnotatedFuncName.str()].push_back(annotStr.str());
              }
            }
          }
        }
      }
    }
  }
}

void computeTransitiveOverrides(const std::string& key, std::unordered_set<std::string>& visited,
                                std::unordered_map<std::string, std::vector<std::string>>& transitiveOverrides,
                                std::unordered_map<std::string, std::vector<std::string>>& overrides) {
  std::unordered_set<std::string> resolved;
  if (transitiveOverrides.count(key)) {
    return;
  }

  if (visited.count(key)) {
    return;
  }

  visited.insert(key);

  for (const auto& elem : overrides[key]) {
    resolved.insert(elem);

    computeTransitiveOverrides(elem, visited, transitiveOverrides, overrides);

    auto it = transitiveOverrides.find(elem);
    if (it != transitiveOverrides.end()) {
      resolved.insert(it->second.begin(), it->second.end());
    }
  }

  visited.erase(key);

  transitiveOverrides[key] = std::vector<std::string>(resolved.begin(), resolved.end());
}

std::unordered_map<std::string, std::vector<std::string>> computeOverriddenBy(
    const std::unordered_map<std::string, std::vector<std::string>>& overrides) {
  std::unordered_map<std::string, std::vector<std::string>> overriddenBy;
  for (const auto& [derived, bases] : overrides) {
    for (const auto& base : bases) {
      overriddenBy[base].push_back(derived);
    }
  }
  return overriddenBy;
}

void computeTransitiveClosure(std::unordered_map<std::string, std::vector<std::string>>& overrides) {
  std::unordered_map<std::string, std::vector<std::string>> transitiveOverrides;
  for (auto& [key, value] : overrides) {
    std::unordered_set<std::string> visited;

    computeTransitiveOverrides(key, visited, transitiveOverrides, overrides);
  }

  overrides = std::move(transitiveOverrides);
}

void run_overrides(std::unordered_map<std::string, std::vector<std::string>>& overrides,
                   std::unordered_map<std::string, std::vector<std::string>>& overriddenBy, llvm::Module& M) {
  collectAnnotations(overrides, M);
  computeTransitiveClosure(overrides);
  overriddenBy = computeOverriddenBy(overrides);
}
}  // namespace
struct CallBaseVisitor : public llvm::InstVisitor<CallBaseVisitor> {
  CallBaseVisitor(llvm::CallGraph* lcg, PTAType pta) : lcg(lcg), pta(pta), mcg(std::make_unique<metacg::Callgraph>()) {
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

    // Only build signature map if required for PTA
    if (pta == BySignature) {
      for (const auto& func : m.getFunctionList()) {
        signatureFunctionMap[func.getFunctionType()].push_back(&func.getFunction());
      }
    }
  }

  ~CallBaseVisitor() = default;

  void visitCallBase(llvm::CallBase& I) {
    // only pass non-resolved calls to metavirt
    if (I.getCalledFunction() != nullptr)
      return;
    auto* currentFunction = I.getParent()->getParent();
    auto& currentNode = mcg->getSingleNode(currentFunction->getName().str());
    if (metaDataAvail) {
      size_t numAddedCalls = addVirtualCallTargets(I, currentNode);
      // This function pointer was a virtual call base, so we do not need to run the overapproximation
      if (numAddedCalls != 0)
        return;
    }

    // metavirt turned up with nothing
    // --> was function pointer, where we can not get the called function
    if (pta == PTAType::BySignature) {
      const auto& possibleFuncs = signatureFunctionMap[I.getFunctionType()];
      for (const auto& func : possibleFuncs) {
        assert(func);
        auto& childNode = getOrInsertNode(func);
        insertEdge(currentNode, childNode);
      }
    }
  }

  void visitFunction(llvm::Function& F) {
    if (F.isIntrinsic())
      return;

    auto& currentNode = getOrInsertNode(&F);

    auto* lcgNode = lcg->operator[](&F);
    for (auto& [key, elem] : *lcgNode) {
      if (!key.has_value())
        continue;
      if (elem->getFunction() == nullptr)
        continue;
      if (elem->getFunction()->isIntrinsic())
        continue;
      const Function* childFunc = elem->getFunction();
      assert(childFunc->hasName());
      metacg::CgNode& childNode = getOrInsertNode(childFunc);
      insertEdge(currentNode, childNode);
    }
  }

  std::unique_ptr<metacg::Callgraph> takeResult() { return std::move(mcg); }

 private:
  size_t addVirtualCallTargets(CallBase& I, const metacg::CgNode& currentNode) {
    // TODO: Improve this design if we want to support multiple virtual call resolution mechanisms, e.g. with
    //       template policy parameter.
#ifdef HAVE_METAVIRT
    auto vcallData = metavirt::vcall_data_for(&I);

    if (!vcallData.has_value()) {
      return 0;
    }

    if (vcallData->call_targets.empty()) {
      return 0;
    }

    for (const auto& dataPoints : metavirt::fn_names_and_origins(vcallData.value())) {
      auto& childNode = mcg->getOrInsertNode(dataPoints.name.str(), dataPoints.origin.str());
      insertEdge(currentNode, childNode);
      auto md = std::make_unique<metacg::CallTypeMD>(metacg::CallType::VIRTUAL);
      mcg->addEdgeMetaData(currentNode, childNode, std::move(md));
      assert(childNode.getOrigin() == dataPoints.origin);
    }
    return metavirt::fn_names_and_origins(vcallData.value()).size();
#else
    return 0;
#endif
  }

  metacg::CgNode& getOrInsertNode(const llvm::Function* F) {
    bool hasBody = !F->isDeclaration();
    StringRef nameToUse = F->getName();
    std::optional<std::string> origin{};
    if (metaDataAvail && functionInfoMap[F] != nullptr) {
      auto linkageName = functionInfoMap[F]->getLinkageName();
      if (!linkageName.empty()) {
        nameToUse = linkageName;
      }
      origin = functionInfoMap[F]->getFilename().str();
    }

    return mcg->getOrInsertNode(nameToUse.str(), std::move(origin), false, hasBody);
  }

  bool insertEdge(const metacg::CgNode& a, const metacg::CgNode& b) {
    if (mcg->existsEdge(a, b)) {
      return false;
    }
    mcg->addEdge(a, b);
    return true;
  }

  std::unique_ptr<metacg::Callgraph> mcg;
  llvm::CallGraph* lcg;
  PTAType pta;
  bool metaDataAvail = false;
  std::unordered_map<const Function*, const llvm::DISubprogram*> functionInfoMap;
  std::unordered_map<llvm::FunctionType*, std::vector<const Function*>> signatureFunctionMap;
};

void Generator::applyOverrideMetadata(std::unordered_map<std::string, std::vector<std::string>> overrides,
                                      std::unordered_map<std::string, std::vector<std::string>> overriddenBy,
                                      metacg::Callgraph* mcg) {
  auto resolveOrPlaceholder = [&](const std::string& name) -> metacg::CgNode* {
    auto node = mcg->getFirstNode(name);
    if (!node) {
      auto& placeholder = mcg->insert(name, "placeholder", false);
      node = &placeholder;
      metacg::MCGLogger::instance().error("Created placeholder node for unresolved override {}", name);
    }
    return node;
  };

  std::unordered_set<std::string> allKeys;
  allKeys.reserve(overrides.size() + overriddenBy.size());
  for (const auto& [key, _] : overrides)
    allKeys.insert(key);
  for (const auto& [key, _] : overriddenBy)
    allKeys.insert(key);

  for (const auto& key : allKeys) {
    auto* node = resolveOrPlaceholder(key);

    auto overrideMD = std::make_unique<metacg::OverrideMD>();

    if (auto it = overrides.find(key); it != overrides.end()) {
      for (const auto& overriddenName : it->second) {
        overrideMD->overrides.push_back(resolveOrPlaceholder(overriddenName)->getId());
      }
    }

    if (auto it = overriddenBy.find(key); it != overriddenBy.end()) {
      for (const auto& overridingName : it->second) {
        overrideMD->overriddenBy.push_back(resolveOrPlaceholder(overridingName)->getId());
      }
    }

    node->addMetaData(std::move(overrideMD));
  }
}

bool Generator::run(Module& M, ModuleAnalysisManager* MA) {
  auto& cgResult = MA->getResult<CallGraphAnalysis>(M);
  auto cbv = CallBaseVisitor(&cgResult, ptaType);
  cbv.visit(M);

  // Take resulting metacg call graph
  auto mcg = cbv.takeResult();
  std::unordered_map<std::string, std::vector<std::string>> overrides;
  std::unordered_map<std::string, std::vector<std::string>> overridenBy;
  run_overrides(overrides, overridenBy, M);
  applyOverrideMetadata(overrides, overridenBy, mcg.get());

  // Run registered plugin's augmentation
  for (auto& plugin : plugins) {
    metacg::MCGLogger::instance().debug("Running {} augment", plugin->getPluginName());
    plugin->augmentCallGraph(M, *mcg);
  }

  // Run registered plugins consumption
  for (auto& plugin : plugins) {
    metacg::MCGLogger::instance().debug("Running {} consume", plugin->getPluginName());
    plugin->consumeCallGraph(*mcg);
  }

  return false;
}
}  // namespace cage
