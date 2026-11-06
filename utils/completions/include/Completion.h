#pragma once

#include <functional>
#include <optional>
#include <string_view>

namespace metacg::completion {

  using CompletionFunction = std::function<void(std::string_view)>;

  std::optional<int> handleCompletion(int argc, const char** argv, CompletionFunction);

} // namespace metacg::completion
