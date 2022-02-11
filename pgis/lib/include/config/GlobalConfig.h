/**
 * File: GlobalConfig.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

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

class HeuristicSelection {
 public:
  friend std::istream &operator>>(std::istream &is, HeuristicSelection &out) {
    std::string name;
    is >> name;
    if (name.empty()) {
      return is;
    }
    std::transform(name.begin(), name.end(), name.begin(), ::tolower);
    if (name == "statements") {
      out.mode = HeuristicSelectionEnum::STATEMENTS;
    } else if (name == "conditionalbranches") {
      out.mode = HeuristicSelectionEnum::CONDITIONALBRANCHES;
    } else if (name == "conditionalbranches_reverse") {
      out.mode = HeuristicSelectionEnum::CONDITIONALBRANCHES_REVERSE;
    } else if (name == "fp_and_mem_ops") {
      // This one ist still a bit experimental
      out.mode = HeuristicSelectionEnum::FP_MEM_OPS;
    } else if (name == "loopdepth") {
      out.mode = HeuristicSelectionEnum::LOOPDEPTH;
    } else if (name == "global_loopdepth") {
      out.mode = HeuristicSelectionEnum::GlOBAL_LOOPDEPTH;
    } else {
      spdlog::get("errconsole")
          ->error(
              "Invalid input for heuristic selection\n"
              "Valid values: statements, conditionalbranches, conditionalbranches_reverse, fp_and_mem_ops, loopdepth, "
              "global_loopdepth\n"
              "Default: statements");
      // is.setstate(is.rdstate() | std::ios::failbit);
      exit(-1);
      return is;
    }
    return is;
  }

  enum class HeuristicSelectionEnum {
    STATEMENTS,
    CONDITIONALBRANCHES,
    CONDITIONALBRANCHES_REVERSE,
    FP_MEM_OPS,  // // This one ist still a bit experimental
    LOOPDEPTH,
    GlOBAL_LOOPDEPTH,
  };
  HeuristicSelectionEnum mode = HeuristicSelectionEnum::STATEMENTS;
};
using HeuristicSelectionOpt = Opt<HeuristicSelection>;

class CuttoffSelection {
 public:
  friend std::istream &operator>>(std::istream &is, CuttoffSelection &out) {
    std::string name;
    is >> name;
    if (name.empty()) {
      return is;
    }
    std::transform(name.begin(), name.end(), name.begin(), ::tolower);
    if (name == "max") {
      out.mode = CuttoffSelectionEnum::MAX;
    } else if (name == "median") {
      out.mode = CuttoffSelectionEnum::MEDIAN;
    } else if (name == "unique_median") {
      out.mode = CuttoffSelectionEnum::UNIQUE_MEDIAN;
    } else {
      spdlog::get("errconsole")
          ->error(
              "Invalid input for cuttof selection\n"
              "Valid values: max, median, unique_median\n"
              "Default: unique_median");
      // is.setstate(is.rdstate() | std::ios::failbit);
      exit(-1);
      return is;
    }
    return is;
  }

  enum class CuttoffSelectionEnum {
    MAX,
    MEDIAN,
    UNIQUE_MEDIAN,
  };
  CuttoffSelectionEnum mode = CuttoffSelectionEnum::UNIQUE_MEDIAN;
};
using CuttoffSelectionOpt = Opt<CuttoffSelection>;

static const BoolOpt runtimeOnly{"runtime-only", "false"};
static const BoolOpt staticSelection{"static", "false"};
static const BoolOpt modelFilter{"model-filter", "false"};
static const BoolOpt scorepOut{"scorep-out", "false"};
static const BoolOpt ipcgExport{"export", "false"};
static const BoolOpt dotExport{"dot-export", "false"};
static const BoolOpt printUnwoundNames{"dump-unwound-names", "false"};
static const BoolOpt useCallSiteInstrumentation{"use-cs-instrumentation", "false"};
static const BoolOpt cubeShowOnly{"cube-show-only", "false"};

static const IntOpt debugLevel{"debug", "0"};

static const StringOpt extrapConfig{"extrap", ""};
static const StringOpt outDirectory{"out-file", "out"};

static const BoolOpt lideEnabled{"lide", "false"};

static const StringOpt parameterFileConfig{"parameter-file", ""};

static const IntOpt metacgFormat{"metacg-format", "1"};

static const HeuristicSelectionOpt heuristicSelection{"heuristic-selection", "statements"};
static const CuttoffSelectionOpt cuttoffSelection{"cuttoff-selection", "unique_median"};
static const BoolOpt keepUnreachable{"keep-unreachable", "false"};

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
  inline ReTy getAs(const std::string &optionName) const {
    auto opt = getOption(optionName);
    if (opt.has_value()) {
      return std::any_cast<ReTy>(opt);
    }
    return ReTy();
  }

  inline std::any getOption(const std::string &optionName) const {
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

inline options::HeuristicSelection::HeuristicSelectionEnum getSelectedHeuristic() {
  const auto &gConfig = pgis::config::GlobalConfig::get();
  const auto heuristicMode = gConfig.getAs<options::HeuristicSelection>(pgis::options::heuristicSelection.cliName).mode;
  return heuristicMode;
}

}  // namespace configPtr

}  // namespace pgis

#endif
