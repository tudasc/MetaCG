#ifndef PGIS_CONFIG_GLOBALCONFIG_H
#define PGIS_CONFIG_GLOBALCONFIG_H

#include "spdlog/spdlog.h"

#include <any>
#include <string>
#include <unordered_map>
#include <utility>

namespace pgis {
namespace options {

template <typename OptValT>
struct Opt {
  typedef OptValT type;
  const std::string cliName;       // XXX Find better name.
  const std::string defaultValue;  // This is std::string to be easy compatible with cxxopts
};

using BoolOpt = Opt<bool>;
using IntOpt = Opt<int>;
using LongOpt = Opt<long>;
using StringOpt = Opt<std::string>;
using DoubleOpt = Opt<double>;

static const BoolOpt runtimeOnly{"runtime-only", "false"};
static const BoolOpt staticSelection{"static", "false"};
static const BoolOpt modelFilter{"model-filter", "false"};
static const BoolOpt scorepOut{"scorep-out", "false"};
static const BoolOpt ipcgExport{"export", "false"};

static const IntOpt debugLevel{"debug", "0"};

static const StringOpt extrapConfig{"extrap", ""};
static const StringOpt outDirectory{"out-file", "out"};

template <typename OptObject>
struct OptHelper {
  typedef typename std::remove_reference<typename std::remove_cv<OptObject>::type>::type::type type;
};
template <typename T>
using arg_t = typename OptHelper<T>::type;

template <typename T>
inline const std::string optKey(const T &obj) {
  return obj.first;
}
}  // namespace options
namespace config {
class GlobalConfig {
 public:
  inline static GlobalConfig &get() {
    static GlobalConfig instance;
    return instance;
  }

  template <typename ReTy>
  inline ReTy getAs(const std::string &optionName) {
    auto opt = getOption(optionName);
    if (opt.has_value()) {
      return std::any_cast<ReTy>(opt);
    }
    return ReTy();
  }

  inline std::any getOption(const std::string &optionName) {
    try {
      auto opt = configOptions.at(optionName);
      return opt;
    } catch (std::out_of_range &exc) {
      spdlog::get("errconsole")->warn("No option named {}", optionName);
      return std::any();
    }
  }

  template <typename ValType>
  bool putOption(const std::string &optionName, ValType &value) {
    auto exists = (configOptions.find(optionName) != configOptions.end());
    if (exists) {
      spdlog::get("errconsole")->warn("Option named {} already exists.", optionName);
    }

    configOptions.try_emplace(optionName, value);
    return !exists;  // if not exists inserted, else false
  }

 protected:
  GlobalConfig() = default;
  ~GlobalConfig() = default;
  GlobalConfig(const GlobalConfig &other) = delete;
  GlobalConfig(const GlobalConfig &&other) = delete;
  GlobalConfig &operator=(const GlobalConfig &other) = delete;
  GlobalConfig &operator=(const GlobalConfig &&other) = delete;

 private:
  std::unordered_map<std::string, std::any> configOptions;
};
}  // namespace config
}  // namespace pgis

#endif
