/**
 * File: GlobalConfig.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#ifndef PGIS_CONFIG_GLOBALCONFIG_H
#define PGIS_CONFIG_GLOBALCONFIG_H

#include "LoggerUtil.h"
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

template<typename To>
To myCast(const std::string &val) {
  std::istringstream ss {val};
  To _t;
  ss >> _t;
  return _t;
}

using BoolOpt = Opt<bool>;
using IntOpt = Opt<int>;
using DoubleOpt = Opt<double>;
using StringOpt = Opt<std::string>;

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
      metacg::MCGLogger::instance().getErrConsole()->error(
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
      metacg::MCGLogger::instance().getErrConsole()->error(
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

class OverheadSelection {
 public:
  friend std::istream &operator>>(std::istream &is, OverheadSelection &out) {
    std::string name;
    is >> name;
    if (name.empty()) {
      return is;
    }
    std::transform(name.begin(), name.end(), name.begin(), ::tolower);
    if (name == "none") {
      out.mode = OverheadSelectionEnum::None;
    } else if (name == "random") {
      out.mode = OverheadSelectionEnum::Random;
    } else if (name == "time_per_call") {
      out.mode = OverheadSelectionEnum::TimePerCall;
    } else if (name == "time_per_call_keep_small") {
      out.mode = OverheadSelectionEnum::TimePerCallKeepSmall;
    } else {
      metacg::MCGLogger::instance().getErrConsole()->error(
          "Invalid input for overhead algorithm selection\n"
          "Valid values: none, random, time_per_call, time_per_call_keep_small\n"
          "Default: none");
      // The config parser seems to ignore errors
      // is.setstate(is.rdstate() | std::ios::failbit);
      exit(-1);
    }
    return is;
  }
  enum class OverheadSelectionEnum { None, Random, TimePerCall, TimePerCallKeepSmall };
  OverheadSelectionEnum mode = OverheadSelectionEnum::None;
};
using OverheadSelectionOpt = Opt<OverheadSelection>;

class DotExportSelection {
 public:
  friend std::istream &operator>>(std::istream &is, DotExportSelection &out) {
    std::string inVal;
    is >> inVal;
    if (inVal.empty()) {
      return is;
    }
    std::transform(inVal.begin(), inVal.end(), inVal.begin(), ::tolower);
    if (inVal == "none") {
    } else if (inVal == "begin") {
      out.mode = DotExportEnum::BEGIN;
    } else if (inVal == "end") {
      out.mode = DotExportEnum::END;
    } else if (inVal == "all") {
      out.mode = DotExportEnum::ALL;
    }
    return is;
  }

  enum class DotExportEnum { NONE, BEGIN, END, ALL };
  DotExportEnum mode = DotExportEnum::NONE;
};
using DotExportSelectionOpt = Opt<DotExportSelection>;

static const BoolOpt runtimeOnly{"runtime-only", "false"};
static const BoolOpt staticSelection{"static", "false"};
static const BoolOpt modelFilter{"model-filter", "false"};
static const BoolOpt scorepOut{"scorep-out", "false"};
static const BoolOpt ipcgExport{"export", "false"};
static const DotExportSelectionOpt dotExport{"dot-export", "none"};
static const BoolOpt printUnwoundNames{"dump-unwound-names", "false"};
static const BoolOpt useCallSiteInstrumentation{"use-cs-instrumentation", "false"};

// TODO: Currently only implemented for the fill and overhead pass
static const BoolOpt onlyInstrumentEligibleNodes{"only-instrument-eligible-nodes", "false"};
static const BoolOpt cubeShowOnly{"cube-show-only", "false"};

static const IntOpt debugLevel{"debug", "0"};

static const StringOpt extrapConfig{"extrap", ""};
static const StringOpt outBaseDir{"out-dir", "out"};
static const StringOpt cubeFilePath{"cube", ""};

static const BoolOpt lideEnabled{"lide", "false"};

static const StringOpt parameterFileConfig{"parameter-file", ""};

static const IntOpt metacgFormat{"metacg-format", "1"};

// "0" means no consideration of Overhead
static const IntOpt targetOverhead{"target-overhead", "0"};
static const IntOpt prevOverhead{"prev-overhead", "0"};

static const HeuristicSelectionOpt heuristicSelection{"heuristic-selection", "statements"};
static const CuttoffSelectionOpt cuttoffSelection{"cuttoff-selection", "unique_median"};
static const BoolOpt fillGaps{"fill-gaps", "false"};

static const OverheadSelectionOpt overheadSelection{"overhead-selection", "none"};

static const BoolOpt sortDotEdges{"sort-dot-edges", "true"};

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

/**
 * Class that holds all options as specified when PGIS is invoked
 */
class GlobalConfig {
 public:
  inline static GlobalConfig &get() {
    static GlobalConfig instance;
    return instance;
  }

  /**
   * Return the stored value for the passed-in option
   */
  template <typename Ty>
  typename Ty::type getVal(const Ty &opt) const {
    using T = typename Ty::type;
    auto o = getOption(opt.cliName);
    if (o.has_value()) {
      return std::any_cast<T>(o);
    }
    return pgis::options::myCast<typename Ty::type>(opt.defaultValue);
  }

  /**
   * Return the stored value cast explicitly to ReTy
   */
  template <typename ReTy>
  inline ReTy getAs(const std::string &optionName) const {
    auto opt = getOption(optionName);
    if (opt.has_value()) {
      return std::any_cast<ReTy>(opt);
    }
    return ReTy();
  }

  /**
   * Return the stored std::any object for optionName
   */
  inline std::any getOption(const std::string &optionName) const {
    try {
      auto opt = configOptions.at(optionName);
      return opt;
    } catch (std::out_of_range &exc) {
      metacg::MCGLogger::instance().getErrConsole()->warn("No option named {}", optionName);
      return std::any();
    }
  }

  /**
   * Store value for optionName
   */
  template <typename ValType>
  bool putOption(const std::string &optionName, ValType &value) {
    auto exists = (configOptions.find(optionName) != configOptions.end());
    if (exists) {
      metacg::MCGLogger::instance().getErrConsole()->warn("Option named {} already exists.", optionName);
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

inline options::OverheadSelection::OverheadSelectionEnum getSelectedOverheadAlgorithm() {
  const auto &gConfig = pgis::config::GlobalConfig::get();
  const auto overheadMode = gConfig.getAs<options::OverheadSelection>(pgis::options::overheadSelection.cliName).mode;
  return overheadMode;
}

}  // namespace config

}  // namespace pgis

#endif
