
#include "CgNode.h"
#include "CgHelper.h"

#define RENDER_DEPS 0

CgNode::CgNode(std::string function) : epCon({}, {}) {
  this->functionName = function;
  this->parentNodes = CgNodePtrSet();
  this->childNodes = CgNodePtrSet();

  this->spantreeParents = CgNodePtrSet();

  this->line = -1;
  this->state = CgNodeState::NONE;
  this->numberOfUnwindSteps = 0;

  this->runtimeInSeconds = 0.0;
  // Mark the inclusive runtime as uninitialized
  this->inclusiveRuntimeInSeconds = -1.0;
  this->expectedNumberOfSamples = 0L;

  this->numberOfCalls = 0;
  this->uniqueCallPath = false;

  this->numberOfStatements = 0;
  this->wasInPreviousProfile = false;
  this->reachable = false;
}

void CgNode::addChildNode(CgNodePtr childNode) { childNodes.insert(childNode); }

void CgNode::addParentNode(CgNodePtr parentNode) { parentNodes.insert(parentNode); }

void CgNode::removeChildNode(CgNodePtr childNode) { childNodes.erase(childNode); }

void CgNode::removeParentNode(CgNodePtr parentNode) {
  numberOfCallsBy.erase(parentNode);
  parentNodes.erase(parentNode);
}

CgNodePtrSet &CgNode::getMarkerPositions() { return potentialMarkerPositions; }
const CgNodePtrSet &CgNode::getMarkerPositionsConst() const { return potentialMarkerPositions; }
CgNodePtrSet &CgNode::getDependentConjunctions() { return dependentConjunctions; }
const CgNodePtrSet &CgNode::getDependentConjunctionsConst() const { return dependentConjunctions; }

void CgNode::addSpantreeParent(CgNodePtr parentNode) { this->spantreeParents.insert(parentNode); }

bool CgNode::isSpantreeParent(CgNodePtr parentNode) {
  return this->spantreeParents.find(parentNode) != spantreeParents.end();
}

void CgNode::reset() {
  this->state = CgNodeState::NONE;
  this->numberOfUnwindSteps = 0;

  this->spantreeParents.clear();
}

void CgNode::updateNodeAttributes(bool updateNumberOfSamples) {
  // this number will not change
  this->numberOfCalls = getNumberOfCallsWithCurrentEdges();

  // has unique call path
  CgNodePtrSet visitedNodes;
  CgNodePtrSet parents = getParentNodes();
  CgNodePtr uniqueParent = nullptr;
  while (parents.size() == 1) {
    // dirty hack
    uniqueParent = (uniqueParent == nullptr) ? getUniqueParent() : uniqueParent->getUniqueParent();

    if (visitedNodes.find(uniqueParent) != visitedNodes.end()) {
      break;  // this can happen in unconnected subgraphs
    } else {
      visitedNodes.insert(uniqueParent);
    }
    parents = uniqueParent->getParentNodes();
  }
  this->uniqueCallPath = (parents.size() == 0);
  if (updateNumberOfSamples) {
    updateExpectedNumberOfSamples();
  }
}

void CgNode::updateExpectedNumberOfSamples() {
  // expected samples in this function (always round up)
  this->expectedNumberOfSamples = (unsigned long long)((double)CgConfig::samplesPerSecond * runtimeInSeconds + 1);
}
void CgNode::setExpectedNumberOfSamples(unsigned long long samples) { this->expectedNumberOfSamples = samples; }

bool CgNode::hasUniqueCallPath() const { return uniqueCallPath; }

bool CgNode::isLeafNode() const { return getChildNodes().empty(); }
bool CgNode::isRootNode() const { return getParentNodes().empty(); }

bool CgNode::hasUniqueParent() const { return getParentNodes().size() == 1; }
bool CgNode::hasUniqueChild() const { return getChildNodes().size() == 1; }
CgNodePtr CgNode::getUniqueParent() const {
  if (!hasUniqueParent()) {
    std::cerr << "Error: no unique parent." << std::endl;
    exit(1);
  }
  return *(getParentNodes().begin());
}
CgNodePtr CgNode::getUniqueChild() const {
  if (!hasUniqueChild()) {
    std::cerr << "Error: no unique child." << std::endl;
    exit(1);
  }
  return *(getChildNodes().begin());
}

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
        if (childNode->hasUniqueChild() && this->hasUniqueParent()) {
          edgeColor = ", color=blue, fontcolor=blue";
        }

        for (auto parentNode : childNode->getParentNodes()) {
          if (this->functionName == parentNode->getFunctionName()) {
            thisNode = parentNode;
          }
        }

        for (auto cgLoc : childNode->getCgLocation()) {
          if (cgLoc.get_procId() == i) {
            numCalls += cgLoc.get_numCalls();
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
      if (!parentNode->getCgLocation().empty()) {
        std::string edgeColor = "";
        if (isSpantreeParent(parentNode)) {
          edgeColor = ", color=red, fontcolor=red";
        }
        if (parentNode->hasUniqueChild() && this->hasUniqueParent()) {
          edgeColor = ", color=blue, fontcolor=blue";
        }

        for (auto cgLoc : this->getCgLocation()) {
          if (cgLoc.get_procId() == procNum) {
            numCalls += cgLoc.get_numCalls();
          }
        }

        outStream << "\"" << parentNode->getFunctionName() << procNum << "\""
                  << " -> \"" << this->functionName << procNum << "\" [label=" << numCalls << edgeColor << "];"
                  << std::endl;
      }
    }
  }

#if RENDER_DEPS
  for (auto markerPosition : potentialMarkerPositions) {
    outStream << *markerPosition << " -> \"" << this->functionName << "\" [style=dotted, color=grey];" << std::endl;
  }
  for (auto dependentConjunction : dependentConjunctions) {
    outStream << "\"" << this->functionName << "\" -> " << *dependentConjunction << " [style=dotted, color=green];"
              << std::endl;
  }
#endif
}

const CgNodePtrSet &CgNode::getChildNodes() const { return childNodes; }

const CgNodePtrSet &CgNode::getParentNodes() const { return parentNodes; }

void CgNode::addCallData(CgNodePtr parentNode, unsigned long long calls, double timeInSeconds, int threadId,
                         int procId) {
  this->numberOfCallsBy[parentNode] += calls;
  this->runtimeInSeconds += timeInSeconds;
  this->cgLoc.push_back(CgLocation(timeInSeconds, threadId, procId, calls));
}

void CgNode::setState(CgNodeState state, int numberOfUnwindSteps) {
  // TODO i think this breaks something
  if (this->state == CgNodeState::INSTRUMENT_CONJUNCTION && this->state != state) {
    //		std::cerr << "# setState old:" << this->state << " new:" << state <<
    // std::endl;

    if (state == CgNodeState::INSTRUMENT_WITNESS) {
      return;  // instrument conjunction is stronger
    }
  }

  this->state = state;

  if (state == CgNodeState::UNWIND_SAMPLE || state == CgNodeState::UNWIND_INSTR) {
    this->numberOfUnwindSteps = numberOfUnwindSteps;
  } else {
    this->numberOfUnwindSteps = 0;
  }
}

void CgNode::setInclusiveRuntimeInSeconds(double newInclusiveRuntimeInSeconds) {
  inclusiveRuntimeInSeconds = newInclusiveRuntimeInSeconds;
}

double CgNode::getInclusiveRuntimeInSeconds() {
  if (childNodes.size() == 0) {
    inclusiveRuntimeInSeconds = runtimeInSeconds;
  } else if (inclusiveRuntimeInSeconds < .0) {
    double rt = CgHelper::calcInclusiveRuntime(this);
    inclusiveRuntimeInSeconds = rt;
  }
  return inclusiveRuntimeInSeconds;
}

CgNodeState CgNode::getStateRaw() const { return state; }
bool CgNode::isInstrumented() const { return isInstrumentedWitness() || isInstrumentedConjunction(); }
bool CgNode::isInstrumentedWitness() const { return state == CgNodeState::INSTRUMENT_WITNESS; }
bool CgNode::isInstrumentedConjunction() const { return state == CgNodeState::INSTRUMENT_CONJUNCTION; }

bool CgNode::isUnwound() const { return state == CgNodeState::UNWIND_SAMPLE || state == CgNodeState::UNWIND_INSTR; }
bool CgNode::isUnwoundSample() const { return state == CgNodeState::UNWIND_SAMPLE; }
bool CgNode::isUnwoundInstr() const { return state == CgNodeState::UNWIND_INSTR; }

int CgNode::getNumberOfUnwindSteps() const { return numberOfUnwindSteps; }

unsigned long long CgNode::getNumberOfCalls() const { return numberOfCalls; }

unsigned long long CgNode::getNumberOfCallsWithCurrentEdges() const {
  unsigned long long numberOfCalls = 0;
  for (auto n : numberOfCallsBy) {
    numberOfCalls += n.second;
  }

  return numberOfCalls;
}

std::vector<CgLocation> CgNode::getCgLocation() const { return cgLoc; }

unsigned long long CgNode::getNumberOfCalls(CgNodePtr parentNode) { return numberOfCallsBy[parentNode]; }

double CgNode::getRuntimeInSeconds() const { return runtimeInSeconds; }

void CgNode::setRuntimeInSeconds(double newRuntimeInSeconds) { runtimeInSeconds = newRuntimeInSeconds; }

unsigned long long CgNode::getExpectedNumberOfSamples() const { return expectedNumberOfSamples; }

void CgNode::setNumberOfStatements(int numStmts) { numberOfStatements = numStmts; }

int CgNode::getNumberOfStatements() const { return numberOfStatements; }
void CgNode::printMinimal() { std::cout << this->functionName; }

void CgNode::print() {
  std::cout << this->functionName << std::endl;
  for (auto n : childNodes) {
    std::cout << "--" << *n << std::endl;
  }
}

void CgNode::setDominance(CgNodePtr child, double dominance) { dominanceMap[child] = dominance; }

double CgNode::getDominance(CgNodePtr child) { return dominanceMap[child]; }

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

namespace std {
bool less<std::shared_ptr<CgNode>>::operator()(const std::shared_ptr<CgNode> &a,
                                               const std::shared_ptr<CgNode> &b) const {
#if 0
  if(a == nullptr) {
    std::cout << "a null" << std::endl;
    if (b != nullptr) {
      std::cout << b->getFunctionName() << std::endl;
    }
    exit(13);
  }
  if(b == nullptr) {
    std::cout << "b null" << std::endl;
    if (a != nullptr) {
      std::cout << a->getFunctionName() << std::endl;
    }
    exit(14);
  }
#endif
  return a->getFunctionName() < b->getFunctionName();
}

bool less_equal<std::shared_ptr<CgNode>>::operator()(const std::shared_ptr<CgNode> &a,
                                                     const std::shared_ptr<CgNode> &b) const {
  return a->getFunctionName() <= b->getFunctionName();
}

bool equal_to<std::shared_ptr<CgNode>>::operator()(const std::shared_ptr<CgNode> &a,
                                                   const std::shared_ptr<CgNode> &b) const {
  return a->getFunctionName() == b->getFunctionName();
}

bool greater<std::shared_ptr<CgNode>>::operator()(const std::shared_ptr<CgNode> &a,
                                                  const std::shared_ptr<CgNode> &b) const {
  return a->getFunctionName() > b->getFunctionName();
}

bool greater_equal<std::shared_ptr<CgNode>>::operator()(const std::shared_ptr<CgNode> &a,
                                                        const std::shared_ptr<CgNode> &b) const {
  return a->getFunctionName() >= b->getFunctionName();
}
}  // namespace std
