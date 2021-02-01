#ifndef LI_RETRIEVER_H
#define LI_RETRIEVER_H

#include "loadImbalance/LIMetaData.h"
#include <CgNodePtr.h>

namespace LoadImbalance {

/**
 * Retriver-struct required for ipcg-annotation
 */
struct LIRetriever {
  bool handles(const CgNodePtr n);

  LIMetaData value(const CgNodePtr n);

  std::string toolName();
};

}  // namespace LoadImbalance

#endif