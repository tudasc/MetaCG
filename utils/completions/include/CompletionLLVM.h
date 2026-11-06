#pragma once

#include "llvm/Support/CommandLine.h"
#include <string_view>
namespace metacg::completion {
    void completeLLVMOptions(std::string_view partial, const llvm::cl::OptionCategory& category);
}
