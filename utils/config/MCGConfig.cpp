/**
 * File: MCGConfig.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */
#include "config.h"

#include <iostream>

#include "cxxopts.hpp"

int main(int argc, char** argv) {
  cxxopts::Options options("metacg-config", "MetaCG configuration tool");
  options.add_options("commands")("v,version", "Prints the version of this MetaCG installation")(
      "revision", "Prints the revision hash of this MetaCG installation")(
      "prefix", "Prints the installation prefix of this MetaCG installation.")("h,help", "Print help");

  const cxxopts::ParseResult result = options.parse(argc, argv);

  if (result.contains("help")) {
    std::cout << options.help() << std::endl;
    return EXIT_SUCCESS;
  }

  // Exactly one of these is allowed at the same time
  int optCount = result.count("version") + result.count("revision") + result.count("prefix");
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

  return EXIT_SUCCESS;
}
