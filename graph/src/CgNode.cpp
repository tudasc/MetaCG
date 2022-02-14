/**
 * File: CgNode.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "CgNode.h"
#include <loadImbalance/LIMetaData.h>

#define RENDER_DEPS 0

namespace metacg {
using namespace pira;

CgNode::CgNode(std::string function)
    : functionName(function),
      line(-1),
      state(CgNodeState::NONE),
      expectedNumberOfSamples(0L),
      uniqueCallPath(false),
      reachable(false),
      parentNodes(),
      childNodes(),
      spantreeParents() {
  /* Attach meta data container */
  auto bpd = new BaseProfileData();
  bpd->setRuntimeInSeconds(.0);
  bpd->setInclusiveRuntimeInSeconds(.0);
  bpd->setNumberOfCalls(0);
  this->addMetaData(bpd);

  auto pod = new PiraOneData();
  pod->setNumberOfStatements(0);
  pod->setComesFromCube(false);
  this->addMetaData(pod);

  auto lid = new LoadImbalance::LIMetaData();
  lid->setNumberOfInclusiveStatements(0);
  lid->setVirtual(false);
  this->isMarkedVirtual = false;
  this->addMetaData(lid);
}

void CgNode::addChildNode(CgNodePtr childNode) { childNodes.insert(childNode); }

void CgNode::addParentNode(CgNodePtr parentNode) { parentNodes.insert(parentNode); }

void CgNode::removeChildNode(CgNodePtr childNode) { childNodes.erase(childNode); }

void CgNode::removeParentNode(CgNodePtr parentNode) { parentNodes.erase(parentNode); }

bool CgNode::isSpantreeParent(CgNodePtr parentNode) {
  return this->spantreeParents.find(parentNode) != spantreeParents.end();
}

void CgNode::reset() {
  this->state = CgNodeState::NONE;
  this->spantreeParents.clear();
}

void CgNode::updateNodeAttributes(bool updateNumberOfSamples) {
  auto bpd = this->get<BaseProfileData>();
  bpd->setNumberOfCalls(bpd->getNumberOfCallsWithCurrentEdges());
}
bool CgNode::hasUniqueCallPath() const { return uniqueCallPath; }

bool CgNode::isLeafNode() const { return getChildNodes().empty(); }
bool CgNode::isRootNode() const { return getParentNodes().empty(); }

bool CgNode::isSameFunction(CgNodePtr cgNodeToCompareTo) const {
  if (this->functionName.compare(cgNodeToCompareTo->getFunctionName()) == 0) {
    return true;
  }
  return false;
}

std::string CgNode::getFunctionName() const { return this->functionName; }

void CgNode::dumpToDot(std::ofstream &outStream, int procNum) {
  std::shared_ptr<CgNode> thisNode;
  if (parentNodes.empty()) {
    for (auto childNode : childNodes) {
      for (int i = 0; i < procNum; i++) {
        std::string edgeColor = "";
        int numCalls = 0;
        if (isSpantreeParent(childNode)) {
          edgeColor = ", color=red, fontcolor=red";
        }

        for (auto parentNode : childNode->getParentNodes()) {
          if (this->functionName == parentNode->getFunctionName()) {
            thisNode = parentNode;
          }
        }

        outStream << "\"" << this->functionName << procNum << "\""
                  << " -> \"" << childNode->getFunctionName() << i << "\" [label=" << numCalls << edgeColor << "];"
                  << std::endl;
      }
    }
  } else {
    for (auto parentNode : parentNodes) {
      int numCalls = 0;
      if (!parentNode->get<BaseProfileData>()->getCgLocation().empty()) {
        std::string edgeColor = "";
        if (isSpantreeParent(parentNode)) {
          edgeColor = ", color=red, fontcolor=red";
        }

        for (auto cgLoc : this->get<BaseProfileData>()->getCgLocation()) {
          if (cgLoc.getProcId() == procNum) {
            numCalls += cgLoc.getNumCalls();
          }
        }

        outStream << "\"" << parentNode->getFunctionName() << procNum << "\""
                  << " -> \"" << this->functionName << procNum << "\" [label=" << numCalls << edgeColor << "];"
                  << std::endl;
      }
    }
  }
}

const CgNodePtrSet &CgNode::getChildNodes() const { return childNodes; }

const CgNodePtrSet &CgNode::getParentNodes() const { return parentNodes; }

void CgNode::setState(CgNodeState state, int numberOfUnwindSteps) {
  // TODO i think this breaks something
  if (this->state == CgNodeState::INSTRUMENT_CONJUNCTION && this->state != state) {
    if (state == CgNodeState::INSTRUMENT_WITNESS) {
      return;  // instrument conjunction is stronger
    }
  }

  this->state = state;
}
void CgNode::instrumentFromParent(CgNodePtr parent) {
  this->instrumentedParentEdges.insert(parent);
}
bool CgNode::isInstrumented() const {
  return isInstrumentedWitness() || isInstrumentedConjunction() || isInstrumentedCallpath();
}
bool CgNode::isInstrumentedWitness() const { return state == CgNodeState::INSTRUMENT_WITNESS; }
bool CgNode::isInstrumentedConjunction() const { return state == CgNodeState::INSTRUMENT_CONJUNCTION; }
bool CgNode::isInstrumentedCallpath() const { return state == CgNodeState::INSTRUMENT_PATH; }

bool CgNode::isUnwound() const { return state == CgNodeState::UNWIND_SAMPLE || state == CgNodeState::UNWIND_INSTR; }
bool CgNode::isUnwoundSample() const { return state == CgNodeState::UNWIND_SAMPLE; }
bool CgNode::isUnwoundInstr() const { return state == CgNodeState::UNWIND_INSTR; }

void CgNode::print() {
  std::cout << this->functionName << std::endl;
  for (auto n : childNodes) {
    std::cout << "--" << *n << std::endl;
  }
}

void CgNode::setFilename(std::string filename) { this->filename = filename; }

void CgNode::setLineNumber(int line) { this->line = line; }

std::ostream &operator<<(std::ostream &stream, const CgNode &n) {
  stream << "\"" << n.getFunctionName() << "\"";

  return stream;
}

std::ostream &operator<<(std::ostream &stream, const CgNodePtrSet &s) {
  stream << "[ ";
  for (auto node : s) {
    stream << node->getFunctionName() << " | ";
  }
  stream << " ]";

  return stream;
}
}

namespace std {
bool less<std::shared_ptr<metacg::CgNode>>::operator()(const std::shared_ptr<metacg::CgNode> &a,
                                               const std::shared_ptr<metacg::CgNode> &b) const {
  return a->getFunctionName() < b->getFunctionName();
}

bool less_equal<std::shared_ptr<metacg::CgNode>>::operator()(const std::shared_ptr<metacg::CgNode> &a,
                                                     const std::shared_ptr<metacg::CgNode> &b) const {
  return a->getFunctionName() <= b->getFunctionName();
}

bool equal_to<std::shared_ptr<metacg::CgNode>>::operator()(const std::shared_ptr<metacg::CgNode> &a,
                                                   const std::shared_ptr<metacg::CgNode> &b) const {
  return a->getFunctionName() == b->getFunctionName();
}

bool greater<std::shared_ptr<metacg::CgNode>>::operator()(const std::shared_ptr<metacg::CgNode> &a,
                                                  const std::shared_ptr<metacg::CgNode> &b) const {
  return a->getFunctionName() > b->getFunctionName();
}

bool greater_equal<std::shared_ptr<metacg::CgNode>>::operator()(const std::shared_ptr<metacg::CgNode> &a,
                                                        const std::shared_ptr<metacg::CgNode> &b) const {
  return a->getFunctionName() >= b->getFunctionName();
}
}  // namespace std
