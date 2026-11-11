#include "CompletionLLVM.h"
#include <algorithm>

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

void metacg::completion::completeLLVMOptions(std::string_view partial, const llvm::cl::OptionCategory& category) {
  llvm::StringRef prefix(partial.data(), partial.size());

  if (!prefix.starts_with("-")) {
    return;
  }

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

    auto* option = it->getValue();

    auto categories = option->Categories;
    
    if (option->getOptionHiddenFlag() != llvm::cl::NotHidden || std::find(categories.begin(), categories.end(), &category) == categories.end()) {
      return;
    }

    for (const auto& value : getOptionValues(*option)) {
      if (value.starts_with(valuePrefix)) {
        llvm::outs() << "--" << optionName << "=" << value << "\n";
      }
    }

    return;
  }
  for (const auto& entry : options) {
#if LLVM_VERSION_MAJOR >= 22
    llvm::StringRef name = entry.getFirst();
    auto* option = entry.getValue();
#else
    llvm::StringRef name = entry.getKey();
    auto* option = entry.getValue();
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
      llvm::outs() << "--" << name;

      if (!getOptionValues(*option).empty()) {
        llvm::outs() << "=";
      }

    llvm::outs() << "\n";
    }
  }
}
