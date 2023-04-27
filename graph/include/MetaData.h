/**
 * File: MetaData.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */
#ifndef METACG_GRAPH_METADATA_H
#define METACG_GRAPH_METADATA_H

#include "CgNodePtr.h"

#include "LoggerUtil.h"
#include "nlohmann/json.hpp"

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
  static CRTPBase *create(const std::string &s, const nlohmann::json &j) {
    if (data().find(s) == data().end()) {
      MCGLogger::instance().getErrConsole()->template warn(
          "Could not create: {}, the Metadata is unknown in you application", s);
      return nullptr;
    }
    return data().at(s)(j);
  }

  template <class T>
  struct Registrar : CRTPBase {
    friend T;

    static bool registerT() {
      MCGLogger::instance().getConsole()->template trace("Registering {} \n",T::key);
      const auto name = T::key;
      MetaDataFactory::data()[name] = [](const nlohmann::json &j) -> CRTPBase * { return new T(j); };
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
  using FuncType = CRTPBase *(*)(const nlohmann::json &);
  MetaDataFactory() = default;

  static auto &data() {
    static std::unordered_map<std::string, FuncType> s;
    return s;
  }
};

template <class Base>
template <class T>
bool MetaDataFactory<Base>::Registrar<T>::registered = MetaDataFactory<Base>::Registrar<T>::registerT();

struct MetaData : MetaDataFactory<MetaData> {
  explicit MetaData(Key) {}
  static constexpr const char *key = "BaseClass";
  virtual nlohmann::json to_json() const = 0;
  virtual ~MetaData() = default;
};

}  // namespace metacg
#endif  // METACG_GRAPH_METADATA_H