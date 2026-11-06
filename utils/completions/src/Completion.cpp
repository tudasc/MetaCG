#include "Completion.h"
#include <string>

std::optional<int> metacg::completion::handleCompletion(int argc, const char** argv,
                                                        CompletionFunction completionFunction) {
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];

    constexpr std::string_view prefix = "--completion=";

    if (arg.compare(0, prefix.size(), prefix) == 0) {
      std::string partial = arg.substr(prefix.size());

      completionFunction(partial);
      return 0;
    }
  }
  return std::nullopt;
}
