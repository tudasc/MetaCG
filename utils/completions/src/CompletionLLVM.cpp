#include "CompletionLLVM.h"
#include <algorithm>

void metacg::completion::completeLLVMOptions(std::string_view partial, const llvm::cl::OptionCategory& category) {
  llvm::StringRef prefix(partial.data(), partial.size());

  if (!prefix.starts_with('-')) {
    return;
  }

  prefix = prefix.ltrim("-");
  // TODO: Option value completion is currently not supported because llvm::cl
  // does not provide a generic way to retrieve the list of possible values
  // from a registered option.

  auto& options = llvm::cl::getRegisteredOptions();

  for (const auto& entry : options) {
    llvm::StringRef name = entry.getKey();
    const auto* option = entry.getValue();

    // Skip hidden options
    if (option->getOptionHiddenFlag() != llvm::cl::NotHidden) {
      continue;
    }

    // Only include options belonging to CGCollector
    const auto& categories = option->Categories;

    if (std::find(categories.begin(), categories.end(), &category) == categories.end()) {
      continue;
    }

    // Match the typed prefix
    if (name.starts_with(prefix)) {
      llvm::outs() << "--" << name << "\n";
    }
  }
}
