/**
 * File: CallGraphCollectionAction.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include <fstream>
#include <iostream>

#include "MCGManager.h"
#include "io/VersionThreeMCGWriter.h"
#include "io/VersionTwoMCGWriter.h"

#include "CallGraphCollectionAction.h"
#include "CallGraphNodeGenerator.h"
#include "Plugin.h"
#include "SharedDefs.h"
#include "metadata/Internal/ASTNodeMetadata.h"
#include "metadata/Internal/AllAliasMetadata.h"
#include "metadata/Internal/FunctionSignatureMetadata.h"

#include "clang/AST/ASTDumper.h"

void CallGraphCollectorConsumer::HandleTranslationUnit(clang::ASTContext& Context) {
  std::string dumpString;
  llvm::raw_string_ostream llvmStringOstream(dumpString);
  Context.getTranslationUnitDecl()->dump(llvmStringOstream);
  SPDLOG_TRACE("\n{}\n", dumpString);

  // Todo: maybe use multigraph managing and write only after all files have been processed
  auto& mcgm = metacg::graph::MCGManager::get();
  mcgm.resetManager();
  mcgm.addToManagedGraphs("newGraph", std::make_unique<metacg::Callgraph>());
  auto callgraph = mcgm.getCallgraph();
  CallGraphNodeGenerator graphGenerator(callgraph, captureCtorsDtors, captureNewDeleteCalls, captureImplicits, inferCtorsDtors,
                                        standalone, level);

  graphGenerator.TraverseDecl(Context.getTranslationUnitDecl());

  // Augment the graph with alias information if this the only TU we will get
  // if other TUs are present, overestimation needs to be done during merge
  if (level == AliasAnalysisLevel::All) {
    addOverestimationEdges(callgraph);
  }

  SPDLOG_INFO("Sucessfully Created Callgraph");
  SPDLOG_INFO("Running Metadata Collectors");
  for (auto& c : mcs) {
    SPDLOG_DEBUG("Running: {}", c->getPluginName());
    for (const auto& node : callgraph->getNodes()) {
      if (!node.second->has<ASTNodeMetadata>()) {
        // This is because we currently do not attach ast node metadata to function pointers
        continue;
      }
      auto nodeDecl = node.second->get<ASTNodeMetadata>()->getFunctionDecl();
      auto md = c->computeForDecl(nodeDecl);
      if (md) {
        node.second->addMetaData(md);
      }
    }
    c->computeForGraph(callgraph);
  }

  std::unique_ptr<metacg::io::MCGWriter> mcgWriter;

  if (mcgVersion == 2) {
    mcgWriter = std::make_unique<metacg::io::VersionTwoMCGWriter>();
  } else if (mcgVersion == 3) {
    mcgWriter = std::make_unique<metacg::io::VersionThreeMCGWriter>();
  }

  metacg::io::JsonSink js;
  mcgWriter->write(callgraph, js);
  auto& sm = Context.getSourceManager();
  std::string filename = sm.getFileEntryRefForID(sm.getMainFileID())->getName().str();
  filename = filename.substr(0, filename.find_last_of('.')) + ".ipcg";
  SPDLOG_INFO("Writing to file: {}", filename);
  std::ofstream file(filename);

  // Fixme: may be a very expensive copy;
  nlohmann::json newJ = js.getJson();
  if (prune) {
    // Fixme: implement deletion of nodes in metacg, it is ridiculous to do this on strings
    for (auto iter = newJ.at("_CG").begin(); iter != newJ.at("_CG").end();) {
      if (iter.value().at("hasBody") == false && iter.value().at("callers").empty()) {
        SPDLOG_TRACE("Pruning: {}", iter.key());
        newJ.at("_CG").erase(iter++);
      } else {
        ++iter;
      }
    }
  }

  file << newJ << std::endl;
}

void CallGraphCollectorConsumer::addOverestimationEdges(metacg::Callgraph* callgraph) {
  SPDLOG_TRACE("Available nodes:");
  // Build datastructure mapping a signature to all node id's with the same signature
  std::unordered_map<FunctionSignature, std::vector<size_t>> signatureToNodeIdMap;
  for (auto& node : callgraph->getNodes()) {
    if (node.second->has<FunctionSignatureMetadata>()) {
      std::ostringstream stream;
      stream << node.second->get<FunctionSignatureMetadata>()->ownSignature;
      SPDLOG_TRACE("{}'s signature is: {} with hash: {}", node.second->getFunctionName(), stream.str(),
                   std::hash<FunctionSignature>()(node.second->get<FunctionSignatureMetadata>()->ownSignature));
      signatureToNodeIdMap[node.second->get<FunctionSignatureMetadata>()->ownSignature].push_back(node.first);
    } else {
      SPDLOG_ERROR("{} has no signature attached", node.second->getFunctionName());
    }
  }

  // This is makes it so a potential function: foo(int (*funcptr)());
  // will potentially call all functions with the signature int (void)
  // This is behaviour that was implemented in the classic cgcollector
  SPDLOG_INFO("Overestimate parameter function pointers from body-less functions");
  for (auto& node : callgraph->getNodes()) {
    // if the node has no body, we need to estimate potential callers via its parameters
    if (!node.second->getHasBody() && node.second->has<ASTNodeMetadata>()) {
      auto md = node.second->get<ASTNodeMetadata>()->getFunctionDecl();
      for (size_t i = 0; i < md->getNumParams(); i++) {
        if (md->getParamDecl(i)->getType()->isPointerType() && md->getParamDecl(i)
                                                                   ->getType()
                                                                   ->getPointeeType()
                                                                   .getDesugaredType(md->getASTContext())
                                                                   .IgnoreParens()
                                                                   ->isFunctionProtoType()) {
          auto protoType = clang::cast<clang::FunctionProtoType>(
              md->getParamDecl(i)->getType()->getPointeeType().getDesugaredType(md->getASTContext()).IgnoreParens());

          SPDLOG_TRACE("The Node: {} has a pointer to a function as its {}th param", node.second->getFunctionName(), i);
          FunctionSignature functionSignature;
          functionSignature.retType = protoType->getReturnType().getAsString();
          std::transform(protoType->getParamTypes().begin(), protoType->getParamTypes().end(),
                         std::back_inserter(functionSignature.paramTypes),
                         [](auto type) { return type.getAsString(); });
          functionSignature.possibleFuncNames.emplace_back("");
          node.second->getOrCreateMD<AllAliasMetadata>()->mightCall.push_back(functionSignature);
        }
      }
    }
  }

  // only if we got told this is the only file, we add the edges now
  // otherwise the edges need to be added once merges have been completed
  if (!standalone) {
    return;
  }

  SPDLOG_INFO("Interfunction overestimation begins");
  // For all nodes, get the signature of a possible call target and add all functions with same signature as edges
  for (auto& node : callgraph->getNodes()) {
    SPDLOG_TRACE("Estimating for node {}", node.second->getFunctionName());
    if (node.second->has<AllAliasMetadata>()) {
      for (const auto& pCallTarget : node.second->get<AllAliasMetadata>()->mightCall) {
        std::ostringstream stream;
        stream << pCallTarget;
        SPDLOG_DEBUG("Node: {} might call: {} with hash: {}", node.second->getFunctionName(), stream.str(),
                     std::hash<FunctionSignature>()(pCallTarget));
        if (signatureToNodeIdMap.find(pCallTarget) == signatureToNodeIdMap.end()) {
          SPDLOG_DEBUG("No function with matching signature found");
          continue;
        }
        for (const auto& id : signatureToNodeIdMap.at(pCallTarget)) {
          SPDLOG_DEBUG("Found function with matching signature: {}", callgraph->getNode(id)->getFunctionName());
          callgraph->addEdge(node.first, id);
        }
      }
    }
  }
}
