#include "CompletionLLVM.h"
#include <algorithm>
#include <llvm/Support/CommandLine.h>

llvm::SmallVector<llvm::StringRef, 8> getOptionValues(llvm::cl::Option& option) {
  llvm::SmallVector<llvm::StringRef, 8> values;

  // If the ArgStr is empty, option.getExtraOptionNames outputs the values to a given vector
  const llvm::StringRef argStr = option.ArgStr;
  option.ArgStr = llvm::StringRef();
  option.getExtraOptionNames(values);
  option.ArgStr = argStr;
  // TODO: one can technically have a value that is the empty string. This could break autocomplete
  return values;
}

void metacg::completion::completeLLVMOptions(std::string_view partial, llvm::cl::OptionCategory& category) {
  llvm::StringRef prefix(partial.data(), partial.size());
  llvm::cl::HideUnrelatedOptions(category);

  if (!prefix.starts_with("-")) {
    return;
  }
  const bool singleDash = prefix.starts_with("-") && !prefix.starts_with("--");
  const bool doubleDash = prefix.starts_with("--");
  const bool shortOption = prefix.starts_with("-") && !doubleDash && prefix.size() > 1;

  prefix = prefix.ltrim("-");

  auto& options = llvm::cl::getRegisteredOptions();

  const auto equalPos = prefix.find('=');
  if (equalPos != llvm::StringRef::npos) {
    llvm::StringRef optionName = prefix.substr(0, equalPos);
    llvm::StringRef valuePrefix = prefix.substr(equalPos + 1);

    optionName = optionName.ltrim('-');

    auto it = options.find(optionName);

    if (it == options.end()) {
      return;
    }

    auto* option = it->second;

    if ((singleDash && optionName.size() != 1) || (doubleDash && optionName.size() == 1)) {
      return;
    }

    for (const auto& value : getOptionValues(*option)) {
      const auto optionPrefix = optionName.size() == 1 ? "-" : "--";
      if (value.starts_with(valuePrefix)) {
        llvm::outs() << optionPrefix << optionName << "=" << value << "\n";
      }
    }

    return;
  }
  for (const auto& entry : options) {
#if LLVM_VERSION_MAJOR >= 22
    llvm::StringRef optionName = entry.getFirst();
#else
    llvm::StringRef optionName = entry.getKey();
#endif

    auto* option = entry.second;

    // Skip hidden options
    if (option->getOptionHiddenFlag() != llvm::cl::NotHidden) {
      continue;
    }

    if ((doubleDash && optionName.size() == 1) || (shortOption && optionName.size() != 1)) {
      continue;
    }

    const auto optionPrefix = optionName.size() == 1 ? "-" : "--";

    // Match the typed prefix
    if (optionName.starts_with(prefix)) {
      llvm::outs() << optionPrefix << optionName;

      if (!getOptionValues(*option).empty()) {
        llvm::outs() << "=";
      }

      llvm::outs() << "\n";
    }
  }
}
