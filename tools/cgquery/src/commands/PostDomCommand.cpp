/**
 * File: PostDomCommand.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */
#include "commands/PostDomCommand.h"
#include "DominatorAnalysis.h"
#include "cxxopts.hpp"
#include "Utils.h"

int PostDomCommand::run(const std::vector<char*>& args) {
  cxxopts::Options options("cgquery dom", "Postdominator analysis");
  options.add_options()("n,node", "Target node", cxxopts::value<std::string>())(
      "e,exit", "Exit node", cxxopts::value<std::string>())("input", "Call graph file", cxxopts::value<std::string>())(
      "h,help", "Print help");

  options.parse_positional({"input"});
  options.positional_help("<input_file>");

  try {
    auto result = options.parse(static_cast<int>(args.size()), args.data());

    if (result.count("help")) {
      std::cout << options.help() << "\n";
      return ExitCode::Success;
    }

    if (!result.count("node") || !result.count("exit")) {
      std::cerr << "Specify target node and entry node.\n";
      std::cerr << options.help();
      return ExitCode::Success;
    }

    std::string cg_name = result["input"].as<std::string>();
    auto fs = metacg::io::FileSource(cg_name);
    auto reader = metacg::io::createReader(fs);
    std::unique_ptr<metacg::Callgraph> cg = reader->read();

    std::string nodeName = result["node"].as<std::string>();
    std::string refName = result["exit"].as<std::string>();

    auto* targetNodePtr = findNode(cg.get(), nodeName, "Target node");
    auto* refNodePtr = findNode(cg.get(), refName, "Exit node");

    if (!targetNodePtr) {
      std::cerr << "Node '" << nodeName << "' not found in CG. \n" << std::endl;
      return ExitCode::Error;
    }
    if (!refNodePtr) {
      std::cerr << "Node '" << refName << "' not found in CG. \n" << std::endl;
      return ExitCode::Error;
    }

    std::unordered_map<const metacg::CgNode*, metacg::analysis::DomData<metacg::CgNode>> dom =
        metacg::analysis::computeDominators<metacg::CgNode, metacg::Callgraph, metacg::analysis::TraverseDir::Backward>(
            *cg, *refNodePtr);

    const auto& data = dom[targetNodePtr];
    if (!data.initialized) {
      std::cerr << "Node '" << nodeName << "' is not reachable from entry node '" << refName
                << "' in post-dominator analysis.\n";
      return ExitCode::Error;
    }

    for (const auto* dPtr : data.Doms) {
      std::cout << dPtr->getFunctionName() << "\n";
    }
  } catch (const cxxopts::exceptions::exception& e) {
    std::cerr << "Error: " << e.what() << "\n";
    std::cerr << options.help() << "\n";
    return ExitCode::Error;
  }

  return ExitCode::Success;
}
