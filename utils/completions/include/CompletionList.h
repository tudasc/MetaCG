#pragma once

#include <string_view>
#include <vector>

namespace metacg::completion {
void completeList(std::string_view partial, std::vector<std::string_view> options);
}
