#ifndef CUBECALLGRAPHTOOL_PROXIMITYMEASUREESTIMATORPHASE_H
#define CUBECALLGRAPHTOOL_PROXIMITYMEASUREESTIMATORPHASE_H

#include "Callgraph.h"
#include "CubeReader.h"
#include "EstimatorPhase.h"

class ProximityMeasureEstimatorPhase : public EstimatorPhase {
 public:
  ProximityMeasureEstimatorPhase(std::string filename);
  virtual void modifyGraph(CgNodePtr mainMethod) override;

  /**
   * Calculates for each node n in profile starting from origFunc
   * val = sum_{F}(childrenPreserved(F, G))/#Func
   * with F being all functions in the subtree starting at origFunc
   * and G being all corresponding functions in the filtered profile
   *
   * \returns a percentage value how many calls were preserved
   */
  double childrenPreservedMetric(CgNodePtr origFunc);
  double portionOfRuntime(CgNodePtr node);

  /**
   * For node it calculates the dominance of the children(node) wrt node.
   * The dominance is defined as
   *
   * dom_{node}(child(node)) = T_i(node) / * (1/calls_{node}(child(node)) * T_i(child(node))
   *
   * Thus the dominance determines to which degree a child function is
   * responsible for the runtime of a parent functions
   */
  std::map<CgNodePtr, double> buildDominanceMap(CgNodePtr node);

  /**
   * The severity is defined as
   *
   * sev_{node}(child(node)) = T_i(child(node)) * calls_{node}(child(node))
   *
   */
  std::map<CgNodePtr, double> buildSeverityMap(CgNodePtr node);

  void printReport() override;

 private:
  double childrenPreserved(CgNodePtr orig, CgNodePtr filtered);
  void prepareList(std::set<CgNodePtr> &worklist, CgNodePtr mainM);
  void prepareListOneLevel(std::set<CgNodePtr> &worklist, CgNodePtr root);
  /**
   * Returns a pair (inclusive runtime for node, accumulated runtime for
   * children(node)
   */
  std::pair<double, double> getInclusiveAndChildrenRuntime(CgNodePtr node);
  CgNodePtr getCorrespondingComparisonNode(const CgNodePtr node);

  std::map<CgNodePtr, double> domMap;
  std::map<CgNodePtr, double> sevMap;

  std::string filename;
  CallgraphManager compareAgainst;
  std::set<CgNodePtr> workQ;
};

#endif
