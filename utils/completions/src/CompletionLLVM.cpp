#include "CompletionLLVM.h"
#include <algorithm>

void metacg::completion::completeLLVMOptions(std::string_view partial, const llvm::cl::OptionCategory& category) {
  llvm::StringRef prefix(partial.data(), partial.size());

  if (!prefix.starts_with("-")) {
    return;
  }

  prefix = prefix.ltrim("-");
llvm::SmallVector<llvm::StringRef, 8> getOptionValues(llvm::cl::Option& option) {
  llvm::SmallVector<llvm::StringRef, 8> values;

  //If the ArgStr is empty, option.getExtraOptionNames outputs the values to a given vector
  const llvm::StringRef argStr = option.ArgStr;
  option.ArgStr = llvm::StringRef();
  option.getExtraOptionNames(values);
  option.ArgStr = argStr;
  //TODO: one can technically have a value that is the empty string. This could break autocomplete
  return values;
}

  auto& options = llvm::cl::getRegisteredOptions();

  for (const auto& entry : options) {
#if LLVM_VERSION_MAJOR > 20
    llvm::StringRef name = entry.getFirst();
    const auto* option = entry.getSecond();
#else
    llvm::StringRef name = entry.getKey();
    const auto* option = entry.getValue();
#endif

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
