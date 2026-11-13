#pragma once

#include <cxxopts.hpp>
#include <string_view>

namespace metacg::completion {
void completeCXXOptsOptions(std::string_view partial, const cxxopts::Options& options);
}
