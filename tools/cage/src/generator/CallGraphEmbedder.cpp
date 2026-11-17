/**
 * File: CallGraphEmbedder.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "cage/generator/CallGraphEmbedder.h"

#include "llvm/IR/Constants.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"

#include "metacg/config.h"
#include "metacg/io/MCGWriter.h"
#include "metacg/io/VersionFourMCGWriter.h"

namespace cage {

using namespace llvm;

static void embed(Module& M, const std::string& sectionName, const std::string& cgStr) {
  LLVMContext& C = M.getContext();

  auto* CgStrData = ConstantDataArray::getString(C, cgStr);

  auto* GV = new GlobalVariable(M, CgStrData->getType(), true, GlobalValue::PrivateLinkage, CgStrData, "__cage_cg");

  GV->setSection(sectionName);
  GV->setAlignment(Align(1));

  appendToCompilerUsed(M, GV);
}

void GraphEmbedder::consumeCallGraph(const metacg::Callgraph& graph) {
  metacg::io::JsonSink jsSink;
  metacg::io::VersionFourMCGWriter mcgw({{4, 0}, {"CaGe", 0, 1, MetaCG_GIT_SHA}}, true, true);
  mcgw.write(&graph, jsSink);

  llvm::outs() << "Embedding generated call graph into ELF section " << sectionName << "\n";

  std::stringstream ss;

  ss << jsSink.getJson().dump();
  ss.flush();
  embed(M, sectionName, ss.str());
}

}  // namespace cage
