#pragma once

#include <flang/Parser/parse-tree.h>
#include <fmt/core.h>
#include <spdlog/spdlog.h>

template <>
struct fmt::formatter<Fortran::parser::CharBlock> {
  constexpr auto parse(fmt::format_parse_context& ctx) { return ctx.begin(); }

  template <typename FormatContext>
  auto format(const Fortran::parser::CharBlock& cb, FormatContext& ctx) const {
    return fmt::format_to(ctx.out(), "{}", std::string_view(cb.begin(), cb.size()));
  }
};

// aligned logger that formats debug messages with a : . To make it more readable.
class AL {
 public:
  static AL* getInstance() {
    static AL instance;
    return &instance;
  }

  template <typename... Args>
  void debug(const std::string& fmt_str, Args&&... args) {
    std::string formatted = fmt::format(fmt_str, std::forward<Args>(args)...);
    size_t colon_pos = formatted.find(':');

    if (colon_pos != std::string::npos) {
      entries.emplace_back(formatted, colon_pos);
      max_key_width = std::max(max_key_width, colon_pos);
    } else {
      entries.emplace_back(formatted, std::string::npos);
    }
  }

  void error(const std::string& fmt_str) { spdlog::error("{}", fmt_str); }

  void flush() {
    for (const auto& [line, colon_pos] : entries) {
      if (colon_pos == std::string::npos) {
        spdlog::debug("{}", line);  // no colon
      } else {
        std::string key = line.substr(0, colon_pos);
        std::string rest = line.substr(colon_pos);  // includes :

        std::string padded_key = fmt::format("{:<{}}", key, max_key_width);
        spdlog::debug("{}{}", padded_key, rest);
      }
    }
    entries.clear();
    max_key_width = 0;
  }

 private:
  AL() = default;

  std::vector<std::pair<std::string, size_t>> entries;
  size_t max_key_width = 0;
};
