#include "CompletionList.h"
#include <iostream>

void metacg::completion::completeList(std::string_view partial, std::vector<std::string_view> options) {
  if (partial.empty() || partial.front() != '-') {
    return;
  }

  while (!partial.empty() && partial.front() == '-') {
    partial.remove_prefix(1);
  }

  for (const auto& option : options) {
    if (option.compare(0, partial.size(), partial) == 0) {
      std::cout << "--" << option << '\n';
    }
  }
}
