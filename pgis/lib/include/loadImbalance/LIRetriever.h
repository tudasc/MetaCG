/**
 * File: LIRetriever.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at https://github.com/tudasc/metacg/LICENSE.txt
 */

#ifndef LI_RETRIEVER_H
#define LI_RETRIEVER_H

#include "loadImbalance/LIMetaData.h"
#include "libIPCG/MetaDataHandler.h"
#include <CgNodePtr.h>

#include "nlohmann/json.hpp"

namespace LoadImbalance {

/**
 * Retriver-struct required for ipcg-annotation
 */
struct LIRetriever {
  bool handles(const CgNodePtr n);

  LIMetaData value(const CgNodePtr n);

  std::string toolName();
};

class LoadImbalanceMetaDataHandler : public MetaCG::io::retriever::MetaDataHandler {
  public:
    bool handles(const CgNodePtr n) const {return false;};
    const std::string toolName() const override {return toolname;}
    void read(const nlohmann::json &j, const std::string &functionName) override;
  private:
    const std::string toolname{"LIData"};
};


}  // namespace LoadImbalance

#endif
