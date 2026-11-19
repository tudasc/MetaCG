#include "CompletionCxxopts.h"
#include <iostream>

void metacg::completion::completeCXXOptsOptions(std::string_view partial, const cxxopts::Options& options) {
  // Ignore leading dashes: "--ver" and "-ver" both match "verbose".
  while (!partial.empty() && partial.front() == '-') {
    partial.remove_prefix(1);
  }

  for (const auto& group : options.groups()) {
    for (const auto& details : options.group_help(group).options) {
      // An option can have several long names. Short-only options have none,
      // so they are skipped automatically.
      for (const std::string& name : details.l) {
        // Match the typed prefix.
        if (name.compare(0, partial.size(), partial) == 0) {
          std::cout << "--" << name << '\n';
        }
      }
    }
  }
}
