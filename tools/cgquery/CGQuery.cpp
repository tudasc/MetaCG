#include "Callgraph.h"
#include "CgNode.h"
#include "CgTypes.h"
#include "LoggerUtil.h"
#include "ReachabilityAnalysis.h"
#include "io/MCGReader.h"
#include <cxxopts.hpp>
#include <iostream>


bool is_number(const std::string& s)
{
    return !s.empty() &&
           std::all_of(s.begin(), s.end(),
               [](unsigned char c) { return std::isdigit(c); });
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: cgquery <command> [options] <input_file>\n";
        return 1;
    }

    std::string command = argv[1];
    std::string cg_name = argv[argc-1];

    if (command == "help" || command == "-h" || command == "--help") {
        std::cout << "Usage: cgquery <command> [options] <input_file>\n";
        std::cout << "Available commands:\n";
        std::cout << "  reaches   Query reachable nodes or check reachability\n";
        return 0;
    }

    if (argc < 3) {
        std::cerr << "Need to specify command and input file." << std::endl;
        std::cerr << "Use cgquery help for available commands." << std::endl;
        return 1;
    }
    // Build argv vector including program name for cxxopts
    std::vector<char*> args;
    args.push_back(argv[0]); // Program name
    for (int i = 2; i < argc; ++i) {
        args.push_back(argv[i]);
    }

    if (command == "reaches") {
        cxxopts::Options options("cgquery reaches", "Query reachable nodes or check if a node is reachable from a certain node");
        options.add_options()
            ("s,source", "Start node", cxxopts::value<std::string>())
            ("t,to", "Target node", cxxopts::value<std::string>())
            ("h,help", "Print help")
            ("input", "Call graph file", cxxopts::value<std::string>());

        options.parse_positional({"input"});
        options.positional_help("<input_file>");

        auto result = options.parse(static_cast<int>(args.size()), args.data());

        if (result.count("help")) {
            std::cout << "help" << std::endl;
            std::cout << options.help() << "\n";
            return 0;
        }

        if (!result.count("source")) {
            std::cerr << "Please specify the source node" << std::endl;
            std::cout << options.help() << std::endl;
            return 2;
        }

        auto fs = metacg::io::FileSource(cg_name);
        std::unique_ptr<metacg::io::MCGReader> r1 = metacg::io::createReader(fs);
        auto cg = r1->read();



        auto sourceStr = result["source"].as<std::string>();
        metacg::CgNode* sourceNode = nullptr;

        if (is_number(sourceStr)) {
            sourceNode = cg->getNode(std::stoul(sourceStr));
        } else {
            if (cg->countNodes(sourceStr) > 1) {
                metacg::MCGLogger::logWarn(
                    "To node name '" + sourceStr +
                    "' is not unique; using first matching node. "
                    "Please provide a unique node ID.");

            };
            sourceNode = cg->getFirstNode(sourceStr);
        }
        if (!sourceNode) {
            std::cerr << "Node '" << result["source"].as<std::string>() << "' not found in CG.\n";
            return 1;
        }

        metacg::analysis::ReachabilityAnalysis reachabilityAnalysis(cg.get());

        if (result.count("to")) {
            std::string toName = result["to"].as<std::string>();
            metacg::CgNode* toNode = nullptr;
            if (is_number(toName)) {
                toNode = cg->getNode(std::stoul(toName));
            } else {
                if (cg->countNodes(toName) > 1) {
                    metacg::MCGLogger::logWarn(
                        "To node name '" + toName +
                        "' is not unique; using first matching node. "
                        "Please provide a unique node ID.");
                };
                toNode = cg->getFirstNode(toName);
            }

            if (!toNode) {
                std::cerr << "Entry node '" << toName << "' not found in CG.\n";
                return 1;
            }

            return reachabilityAnalysis.existsPathBetween(sourceNode, toNode, true) ? 0 : 1;
        }
        else {
            auto reachableNodes = reachabilityAnalysis.getReachableNodesFrom(sourceNode, true);
            std::cout << "Reachable nodes:\n";

            for (const auto n : reachableNodes) {
                std::cout << n->getFunctionName() << '\n';
            }

            return 0;
        }
    }

    return 0;
}

