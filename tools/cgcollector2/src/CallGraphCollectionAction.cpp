/**
 * File: CallGraphCollectionAction.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "cgcollector2/CallGraphCollectionAction.h"
#include "cgcollector2/CallGraphNodeGenerator.h"
#include "cgcollector2/SharedDefs.h"
#include "cgcollector2/interface/CGC2Plugin.h"
#include "cgcollector2/metadata/Internal/ASTNodeMetadata.h"
#include "cgcollector2/metadata/Internal/AllAliasMetadata.h"
#include "cgcollector2/metadata/Internal/FunctionSignatureMetadata.h"
#include "metacg/LoggerUtil.h"
#include "metacg/MCGBaseInfo.h"

#include "metacg/MCGManager.h"
#include "metacg/io/VersionFourMCGWriter.h"
#include "metacg/io/VersionTwoMCGWriter.h"

#include "clang/AST/ASTDumper.h"

#include <fstream>

void CallGraphCollectorConsumer::HandleTranslationUnit(clang::ASTContext& Context) {
  std::string dumpString;
  llvm::raw_string_ostream llvmStringOstream(dumpString);
  Context.getTranslationUnitDecl()->dump(llvmStringOstream);
  metacg::MCGLogger::instance().getConsole()->trace("\n{}\n", dumpString);

  // Todo: maybe use multigraph managing and write only after all files have been processed
  auto& mcgm = metacg::graph::MCGManager::get();
  mcgm.resetManager();
  mcgm.addToManagedGraphs("newGraph", std::make_unique<metacg::Callgraph>());
  auto callgraph = mcgm.getCallgraph();
  CallGraphNodeGenerator graphGenerator(callgraph, captureCtorsDtors, captureNewDeleteCalls, captureImplicits,
                                        inferCtorsDtors, standalone, level, sema);

  graphGenerator.TraverseDecl(Context.getTranslationUnitDecl());

  // Augment the graph with alias information if this the only TU we will get
  // if other TUs are present, overestimation needs to be done during merge
  if (level == AliasAnalysisLevel::All) {
    addOverestimationEdges(callgraph);
  }

  metacg::MCGLogger::logInfo("Sucessfully Created Callgraph");
  metacg::MCGLogger::logInfo("Running Metadata Collectors");
  for (auto& c : mcs) {
    metacg::MCGLogger::instance().getConsole()->debug("Running: {}", c->getPluginName());
    for (const auto& node : callgraph->getNodes()) {
      if (!node->has<ASTNodeMetadata>()) {
        // This is because we currently do not attach ast node metadata to function pointers
        continue;
      }
      auto nodeDecl = node->get<ASTNodeMetadata>()->getFunctionDecl();
      auto md = c->computeForDecl(nodeDecl);
      if (md) {
        node->addMetaData(std::move(md));
      }
    }
    c->computeForGraph(callgraph);
  }

  std::unique_ptr<metacg::io::MCGWriter> mcgWriter;

  switch (mcgVersion) {
    case 1:
      metacg::MCGLogger::logWarn("This tool can not generate output files in the V1 format, using V2 instead");
      __attribute__((fallthrough));
    case 2:
      mcgWriter = std::make_unique<metacg::io::VersionTwoMCGWriter>(metacg::MCGFileInfo(
          {2, 0},
          {std::string("CGCollector2"), MetaCG_VERSION_MAJOR, MetaCG_VERSION_MINOR, MetaCG_GIT_SHA}));
      break;
    case 3:
      metacg::MCGLogger::logWarn("V3 format was removed and is currently not supported, using V4 instead");
      __attribute__((fallthrough));
    case 4:
      mcgWriter = std::make_unique<metacg::io::VersionFourMCGWriter>(metacg::MCGFileInfo(
          {4, 0},
          {std::string("CGCollector2"), MetaCG_VERSION_MAJOR, MetaCG_VERSION_MINOR, MetaCG_GIT_SHA}));
      break;
    default:
      assert(false && "The selected output format is not recognized");
      metacg::MCGLogger::logWarn("The selected output format {} is not recognized. Using default format (V2) instead",
                                 mcgVersion);
      mcgWriter = std::make_unique<metacg::io::VersionTwoMCGWriter>();
  }

  metacg::io::JsonSink js;
  mcgWriter->write(callgraph, js);
  std::string filename;

  if (cgout.empty()) {
    auto& sm = Context.getSourceManager();
    filename = sm.getFileEntryRefForID(sm.getMainFileID())->getName().str();
    filename = filename.substr(0, filename.find_last_of('.')) + ".ipcg";
  } else {
    filename = cgout;
  }
  std::ofstream file(filename);
  if (prune) {
    metacg::MCGLogger::logInfo("Pruning callgraph");
    // Fixme: may be a very expensive copy;
    nlohmann::json newJ = js.getJson();
    // Fixme: implement deletion of nodes in metacg, it is ridiculous to do this on strings
    for (auto iter = newJ.at("_CG").begin(); iter != newJ.at("_CG").end();) {
      //We need to check for callees as well, as bodiless functions can still have calls
      if (iter.value().at("hasBody") == false && iter.value().at("callers").empty() && iter.value().at("callees").empty()) {
        metacg::MCGLogger::instance().getConsole()->trace("Pruning: {}", iter.key());
        newJ.at("_CG").erase(iter++);
      } else {
        ++iter;
      }
    }
    js.setJson(newJ);
  }

  metacg::MCGLogger::logInfo("Writing to file: {}", filename);
  file << js.getJson() << std::endl;
}

void CallGraphCollectorConsumer::addOverestimationEdges(metacg::Callgraph* callgraph) {
  metacg::MCGLogger::instance().getConsole()->trace("Available nodes:");
  // Build datastructure mapping a signature to all node id's with the same signature
  std::unordered_map<FunctionSignature, std::vector<size_t>> signatureToNodeIdMap;
  for (auto& node : callgraph->getNodes()) {
    if (node->has<FunctionSignatureMetadata>()) {
      std::ostringstream stream;
      stream << node->get<FunctionSignatureMetadata>()->ownSignature;
      metacg::MCGLogger::instance().getConsole()->trace(
          "{}'s signature is: {} with hash: {}", node->getFunctionName(), stream.str(),
          std::hash<FunctionSignature>()(node->get<FunctionSignatureMetadata>()->ownSignature));
      signatureToNodeIdMap[node->get<FunctionSignatureMetadata>()->ownSignature].push_back(node->getId());
    } else {
      metacg::MCGLogger::logError("{} has no signature attached", node->getFunctionName());
    }
  }

  // This makes it so a potential function: foo(int (*funcptr)());
  // will potentially call all functions with the signature int (void)
  // This is behaviour that was implemented in the classic cgcollector
  metacg::MCGLogger::logInfo("Overestimate parameter function pointers from body-less functions");
  for (auto& node : callgraph->getNodes()) {
    // if the node has no body, we need to estimate potential callers via its parameters
    if (!node->getHasBody() && node->has<ASTNodeMetadata>()) {
      auto md = node->get<ASTNodeMetadata>()->getFunctionDecl();
      for (size_t i = 0; i < md->getNumParams(); i++) {
        if (md->getParamDecl(i)->getType()->isPointerType() && md->getParamDecl(i)
                                                                   ->getType()
                                                                   ->getPointeeType()
                                                                   .getDesugaredType(md->getASTContext())
                                                                   .IgnoreParens()
                                                                   ->isFunctionProtoType()) {
          auto protoType = clang::cast<clang::FunctionProtoType>(
              md->getParamDecl(i)->getType()->getPointeeType().getDesugaredType(md->getASTContext()).IgnoreParens());

          metacg::MCGLogger::instance().getConsole()->trace(
              "The Node: {} has a pointer to a function as its {}th param", node->getFunctionName(), i);
          FunctionSignature functionSignature;
          functionSignature.retType = protoType->getReturnType().getAsString();
          std::transform(protoType->getParamTypes().begin(), protoType->getParamTypes().end(),
                         std::back_inserter(functionSignature.paramTypes),
                         [](auto type) { return type.getAsString(); });
          functionSignature.possibleFuncNames.emplace_back("");
          node->getOrCreate<AllAliasMetadata>().mightCall.push_back(functionSignature);
        }
      }
    }
  }

  // only if we got told this is the only file, we add the edges now
  // otherwise the edges need to be added once merges have been completed
  if (!standalone) {
    return;
  }

  metacg::MCGLogger::logInfo("Interfunction overestimation begins");
  // For all nodes, get the signature of a possible call target and add all functions with same signature as edges
  for (auto& node : callgraph->getNodes()) {
    metacg::MCGLogger::instance().getConsole()->trace("Estimating for node {}", node->getFunctionName());
    if (node->has<AllAliasMetadata>()) {
      for (const auto& pCallTarget : node->get<AllAliasMetadata>()->mightCall) {
        std::ostringstream stream;
        stream << pCallTarget;
        metacg::MCGLogger::instance().getConsole()->debug("Node: {} might call: {} with hash: {}",
                                                          node->getFunctionName(), stream.str(),
                                                          std::hash<FunctionSignature>()(pCallTarget));
        if (signatureToNodeIdMap.find(pCallTarget) == signatureToNodeIdMap.end()) {
          metacg::MCGLogger::instance().getConsole()->debug("No function with matching signature found");
          continue;
        }
        for (const auto& id : signatureToNodeIdMap.at(pCallTarget)) {
          metacg::MCGLogger::instance().getConsole()->debug("Found function with matching signature: {}",
                                                            callgraph->getNode(id)->getFunctionName());
          callgraph->addEdge(node->getId(), id);
        }
      }
    }
  }
}
