#pragma once

#include "llvm/Support/CommandLine.h"
#include <string_view>
namespace metacg::completion {
void completeLLVMOptions(std::string_view partial, llvm::cl::OptionCategory& category);
}
