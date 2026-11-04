#include "metacg/Callgraph.h"
#include "metacg/MergePolicy.h"
#include "metacg/metadata/CallTypeMD.h"
#include "metacg/metadata/OverrideMD.h"

namespace metacg {
struct VirtualCallResolutionTask {
  void operator()(Callgraph* cg) const {
    for (const auto& [nodePair, namedMD] : cg->getEdges()) {
      const auto callTypeIt = namedMD.find(CallTypeMD::key);
      if (callTypeIt == namedMD.end()) {
        continue;
      }

      const auto* callTypeMD = dynamic_cast<const CallTypeMD*>(callTypeIt->second.get());

      if (!callTypeMD || callTypeMD->callType != CallType::VIRTUAL) {
        continue;
      }

      const auto& [callerId, calleeId] = nodePair;

      const auto caller = cg->getNode(callerId);
      const auto callee = cg->getNode(calleeId);

      if (!caller || !callee) {
        continue;
      }

      const auto& calleeMD = callee->getMetaDataContainer();
      const auto overrideIt = calleeMD.find(OverrideMD::key);

      if (overrideIt == calleeMD.end()) {
        continue;
      }

      const auto* overrideMD = dynamic_cast<const OverrideMD*>(overrideIt->second.get());

      if (!overrideMD) {
        continue;
      }

      for (const auto& overrideId : overrideMD->overriddenBy) {
        const auto node = cg->getNode(overrideId);

        if (!node) {
          continue;
        }

        if (!cg->existsEdge(*caller, *node)) {
          cg->addEdge(*caller, *node);

          auto md = std::make_unique<CallTypeMD>(CallType::VIRTUAL);
          cg->addEdgeMetaData(*caller, *node, std::move(md));
        }
      }
    }
  }
};

}  // namespace metacg
