/**
* File: CGQuery.h
* License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
* https://github.com/tudasc/metacg/LICENSE.txt
*/
#include "commands/PostDomCommand.h"
#include "commands/ReachesCommand.h"
#include "commands/DomCommand.h"
#include "utils.h"

#include <cxxopts.hpp>
#include <iostream>
#include <memory>
#include <ostream>


int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: cgquery <command> [options] <input_file>\n";
        return ExitCode::Usage;
    }

    std::string command = argv[1];

    if (command == "help" || command == "-h" || command == "--help") {
        std::cout << "Usage: cgquery <command> [options] <input_file>\n";
        std::cout << "Available commands:\n";
        std::cout << "  reaches    Query reachable nodes or check reachability\n";
        std::cout << "  dom    Query dominators of a node for a given entry node\n";
        std::cout << "  postdom    Query postdominators of a node for a given exit node\n";
        std::cout << "  help    Show this help";
        return ExitCode::Success;
    }

    if (argc < 3) {
        std::cerr << "Need to specify command and input file." << std::endl;
        std::cerr << "Use cgquery help for available commands." << std::endl;
        return ExitCode::Usage;
    }

    // Build argv vector including program name for cxxopts
    std::vector<char*> args;
    args.push_back(argv[0]); // Program name
    for (int i = 2; i < argc; ++i) {
        args.push_back(argv[i]);
    }

    std::unique_ptr<Command> cmd;

    if (command == "reaches") {
        cmd = std::make_unique<ReachesCommand>();
    } else if (command == "dom") {
        cmd = std::make_unique<DomCommand>();

    } if (command == "postdom") {
        cmd = std::make_unique<PostDomCommand>();
    } else {
        std::cerr << "Unknown command " << command << "." << std::endl;
        return ExitCode::Usage;
    }

    return cmd->run(args);
}

