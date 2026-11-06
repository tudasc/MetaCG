#pragma once

#include <string_view>
#include <cxxopts.hpp>

namespace metacg::completion {
    void completeCXXOptsOptions(std::string_view partial, const cxxopts::Options& options);
}
