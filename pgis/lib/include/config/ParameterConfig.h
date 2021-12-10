/**
 * File: ParameterConfig.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#ifndef PGIS_PARAMETERCONFIG_H
#define PGIS_PARAMETERCONFIG_H

#include "PiraIIConfig.h"
#include "loadImbalance/LIConfig.h"
#include <optional>

namespace pgis::config {

/**
 * Singleton structure containing analysis parameters (e.g. for PIRA II or PIRA LIDe)
 *
 * Parameters for different components (e.g. LIDe) are encapsulated in structs (e.g LoadImbalance::LIConfig).
 * This class contains unique pointers for each component.
 * These pointers can be nullptr iff no corresponding configuration has been provided during the call to PGIS.
 */
class ParameterConfig {
 protected:
  /**
   * Constructor which reads the configuration file for analysis parameters.
   * This is the only way to initialize this construct.
   */
  ParameterConfig();
  ~ParameterConfig() = default;

 private:
  // start of actual configuration objects
  // ====================================
  std::unique_ptr<PiraIIConfig> piraIIconfig = nullptr;
  std::unique_ptr<LoadImbalance::LIConfig> liConfig = nullptr;
  // ===================================
  // end of actual configuration objects

 public:
  ParameterConfig(const ParameterConfig &other) = delete;
  ParameterConfig(const ParameterConfig &&other) = delete;
  ParameterConfig &operator=(const ParameterConfig &other) = delete;
  ParameterConfig &operator=(const ParameterConfig &&other) = delete;

  // Singleton!
  inline static ParameterConfig &get() {
    static ParameterConfig instance;  // lazy!
    return instance;
  }

  // ========
  //  GETTER
  // ========

  std::unique_ptr<PiraIIConfig> &getPiraIIConfig() {
    assert(this->piraIIconfig != nullptr &&
           "ParameterConfig.h: Tried to read analysis parameters for PIRA II which have not been provided.");
    return this->piraIIconfig;
  }

  std::unique_ptr<LoadImbalance::LIConfig> &getLIConfig() {
    assert(this->piraIIconfig != nullptr &&
           "ParameterConfig.h: Tried to read analysis parameters for PIRA LIDe which have not been provided.");
    return this->liConfig;
  }
};
}  // namespace pgis::config
#endif  // PGIS_PARAMETERCONFIG_H
