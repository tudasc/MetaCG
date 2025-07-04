/**
 * File: MetaData.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */
#ifndef METACG_GRAPH_METADATA_H
#define METACG_GRAPH_METADATA_H

#include "LoggerUtil.h"
#include "IdMapping.h"
#include "nlohmann/json.hpp"

// Instance counter to protect meta-data registry against ABI incompatibilities
// Used in `ABICheckedStaticData`.
// Is extern "C" to guarantee identical mangling.
extern "C" {
extern int metacg_RegistryInstanceCounter;
};

namespace metacg {
namespace graph {
class MCGManager;
}

/**
 * This is the common base class for the different user-defined metadata.
 *  that can be attached to the call graph.
 *
 *  A class *must* implement a  static constexpr const char *key that contains the class
 *  name as a string. This is used for registration in the MetaData field of the CgNode.
 */
template <class CRTPBase>
class MetaDataFactory {
 public:
  template <class... T>
  static std::unique_ptr<CRTPBase> create(const std::string& s, const nlohmann::json& j, StrToNodeMapping& strToNode) {
    if (data().find(s) == data().end()) {
      MCGLogger::instance().warn<MCGLogger::LogType::UNIQUE, MCGLogger::Output::ErrConsole>(
          "Could not create {}: the metadata is unknown in your application", s);
      return nullptr;
    }
    return data().at(s)(j, strToNode);
  }

  template <class T>
  struct Registrar : CRTPBase {
    friend T;

    static bool registerT() {
      MCGLogger::instance().getConsole()->trace("Registering {} \n", T::key);
      const auto name = T::key;
      MetaDataFactory::data()[name] = [](const nlohmann::json& j, StrToNodeMapping& strToNode) -> std::unique_ptr<CRTPBase> { return std::make_unique<T>(j, strToNode); };
      return true;
    }
    static bool registered;

   private:
    Registrar() : CRTPBase(Key{}) { (void)registered; }
  };

  friend CRTPBase;

 private:
  class Key {
    Key() = default;
    template <class T>
    friend struct Registrar;
  };
  using FuncType = std::unique_ptr<CRTPBase> (*)(const nlohmann::json&, StrToNodeMapping&);
  MetaDataFactory() = default;

  /**
   * Helper type to create static data (basically, a singleton) that is protected against ABI mismatches.
   * When not using this, ABI incompatibilities (e.g., due to mismatching compilers) can lead to the
   * creation of more than one instances.
   */
  template <typename T>
  struct ABICheckedStaticData {
    T data;

    ABICheckedStaticData(int& instanceCtr) {
      if ((++instanceCtr) != 1) {
        MCGLogger::instance().getErrConsole()->error(
            "Detected multiple instances of the global metadata registry, likely due to ABI compatibility. Custom "
            "metadata will not work properly. To fix this, ensure that the metadata library is build with the same "
            "compiler as the rest of MetaCG.");
      }
    }
  };

  static auto& data() {
    static ABICheckedStaticData<std::unordered_map<std::string, FuncType>> s(metacg_RegistryInstanceCounter);
    return s.data;
  }
};

template <class Base>
template <class T>
bool MetaDataFactory<Base>::Registrar<T>::registered = MetaDataFactory<Base>::Registrar<T>::registerT();

struct MetaData : MetaDataFactory<MetaData> {
  explicit MetaData(Key) {}
  static constexpr const char* key = "BaseClass";
  virtual nlohmann::json toJson(NodeToStrMapping&) const = 0;
  virtual const char* getKey() const = 0;
  virtual void merge(const MetaData&) = 0;
  [[nodiscard]] virtual std::unique_ptr<MetaData> clone() const = 0;
  virtual ~MetaData() = default;
};

}  // namespace metacg
#endif  // METACG_GRAPH_METADATA_H