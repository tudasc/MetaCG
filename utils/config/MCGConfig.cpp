/**
 * File: MCGConfig.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */
#include "metacg/config.h"

#include <iostream>

#include "cxxopts.hpp"

// Setting default to prevent build errors when graph tools are not available.
#ifndef CAGE_PLUGIN
#define CAGE_PLUGIN
#endif

int main(int argc, char** argv) {
  cxxopts::Options options("metacg-config", "MetaCG configuration tool");
  // clang-format off
  options.add_options("commands")("v,version", "Prints the version of this MetaCG installation.")(
      "revision", "Prints the revision hash of this MetaCG installation")(
      "prefix", "Prints the installation prefix of this MetaCG installation.")(
      "cage-ldflags", "Prints the necessary ld flags.")(
      "cage-cflags", "Prints the necessary C compile flags.")(
      "cage-cxxflags", "Prints the necessary C++ compile flags.")(
      "cage-pass-option", "Sets a pass option", cxxopts::value<std::vector<std::string>>())(
      "h,help", "Print help");
  // clang-format on

  const cxxopts::ParseResult result = options.parse(argc, argv);

  if (result.contains("help")) {
    std::cout << options.help() << std::endl;
    return EXIT_SUCCESS;
  }

  bool cageOptionSet = result.count("cage-ldflags") + result.count("cage-cflags") + result.count("cage-cxxflags") +
                       result.count("cage-pass-option");

#ifndef HAVE_CAGE
  if (cageOptionSet) {
    std::cerr << "CaGe is currently not available. Configure with -DMETACG_BUILD_GRAPH_TOOLS to build it.\n";
    return EXIT_FAILURE;
  }
#endif

  // Exactly one of these is allowed at the same time
  int optCount = result.count("version") + result.count("revision") + result.count("prefix") + (cageOptionSet ? 1 : 0);
  if (optCount == 0) {
    std::cerr << "Error: No command specified.\n";
    return EXIT_FAILURE;
  } else if (optCount > 1) {
    std::cerr << "Warning: Multiple mutually exclusive commands specified. Only one of them will be processed.\n";
  }

  if (result.contains("version")) {
    std::cout << MetaCG_VERSION_MAJOR << "." << MetaCG_VERSION_MINOR;
    return EXIT_SUCCESS;
  }

  if (result.contains("revision")) {
    std::cout << MetaCG_GIT_SHA;
    return EXIT_SUCCESS;
  }

  if (result.contains("prefix")) {
    std::cout << INSTALL_PREFIX;
    return EXIT_SUCCESS;
  }

#ifdef HAVE_CAGE
  std::stringstream passOptsStream;
  if (result.count("cage-pass-option")) {
    const auto& passOpts = result["cage-pass-option"].as<std::vector<std::string>>();
    for (auto& opt : passOpts) {
      passOptsStream << " -Wl,-mllvm=" << opt << " ";
    }
  }
  std::string processedPassOpts = passOptsStream.str();

  if (result.contains("cage-cflags")) {
    std::cout << " -flto ";
  }

  if (result.contains("cage-cxxflags")) {
    std::cout << " -flto ";
  }

  if (result.contains("cage-ldflags")) {
    std::cout << " -flto -fuse-ld=lld -Wl,-mllvm=-load=" << CAGE_PLUGIN << " -Wl,--load-pass-plugin=" << CAGE_PLUGIN
              << processedPassOpts << " ";
  }
#endif

  return EXIT_SUCCESS;
}
