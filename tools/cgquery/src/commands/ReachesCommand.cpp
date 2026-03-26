/**
 * File: ReachesCommand.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */
#include "commands/ReachesCommand.h"
#include "CgNode.h"
#include "ReachabilityAnalysis.h"
#include "cxxopts.hpp"
#include "io/MCGReader.h"
#include "Utils.h"
#include <iostream>

int ReachesCommand::run(const std::vector<char*>& args) {
  cxxopts::Options options("cgquery reaches",
                           "Query reachable nodes or check if a node is reachable from a certain node");
  options.add_options()("s,source", "Start node", cxxopts::value<std::string>())(
      "t,to", "Target node", cxxopts::value<std::string>())("h,help", "Print help")("input", "Call graph file",
                                                                                    cxxopts::value<std::string>());

  options.parse_positional({"input"});
  options.positional_help("<input_file>");

  try {
    auto result = options.parse(static_cast<int>(args.size()), args.data());

    if (result.count("help")) {
      std::cout << options.help() << "\n";
      return ExitCode::Success;
    }

    std::string cg_name = result["input"].as<std::string>();
    auto fs = metacg::io::FileSource(cg_name);
    auto reader = metacg::io::createReader(fs);
    std::unique_ptr<metacg::Callgraph> cg = reader->read();

    if (!result.count("source")) {
      std::cerr << "Please specify the source node" << std::endl;
      std::cout << options.help() << std::endl;
      return ExitCode::Usage;
    }

    auto sourceStr = result["source"].as<std::string>();
    metacg::CgNode* sourceNode = nullptr;

    sourceNode = findNode(cg.get(), sourceStr, "Source node");

    if (!sourceNode) {
      std::cerr << "Node '" << result["source"].as<std::string>() << "' not found in CG.\n";
      return ExitCode::Error;
    }

    metacg::analysis::ReachabilityAnalysis reachabilityAnalysis(cg.get());

    if (result.count("to")) {
      std::string toName = result["to"].as<std::string>();
      metacg::CgNode* toNode = findNode(cg.get(), toName, "To node");

      if (!toNode) {
        std::cerr << "Entry node '" << toName << "' not found in CG.\n";
        return ExitCode::Error;
      }

      return !reachabilityAnalysis.existsPathBetween(sourceNode, toNode, true);
    } else {
      auto reachableNodes = reachabilityAnalysis.getReachableNodesFrom(sourceNode, true);
      std::cout << "Reachable nodes:\n";

      for (const auto n : reachableNodes) {
        std::cout << n->getFunctionName() << '\n';
      }

      return ExitCode::Success;
    }
  } catch (const cxxopts::exceptions::exception& e) {
    std::cerr << "Error: " << e.what() << "\n";
    std::cerr << options.help() << "\n";
    return ExitCode::Error;
  }
}
