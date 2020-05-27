#ifndef PIRA_CGNODE_METADATA_H
#define PIRA_CGNODE_METADATA_H

#include "CgNodePtr.h"

#include "ExtrapConnection.h"

namespace pira {

/**
 * This is meant as common base class for the different meta data
 *  that can be attached to the call graph.
 *
 *  A class *must* implement a method static constexpr const char *key() that returns the class
 *  name as a string. This is used for registration in the MetaData field of the CgNode.
 */
struct MetaData {};

/**
 * This class holds basic profile information, e.g., from reading a CUBE
 */
class BaseProfileData : public MetaData {
 public:
  static constexpr const char *key() { return "BaseProfileData"; }

  // Regular profile data
  void addCallData(CgNodePtr parentNode, unsigned long long calls, double timeInSeconds, int threadId, int procId) {
    callFrom[parentNode] += calls;
    timeFrom[parentNode] += timeInSeconds;
    this->timeInSeconds += timeInSeconds;
    this->threadId = threadId;
    this->processId = procId;
  }
  unsigned long long getNumberOfCalls() const { return this->numCalls; }
  void setNumberOfCalls(unsigned long long nrCall) { this->numCalls = nrCall; }

  double getRuntimeInSeconds() const { return this->timeInSeconds; }
  void setRuntimeInSeconds(double newRuntimeInSeconds) { this->timeInSeconds = newRuntimeInSeconds; }
  double getRuntimeInSecondsForParent(CgNodePtr parent) { return this->timeFrom[parent]; }

  [[deprecated("use CgHelper::calInclusiveRuntime for now")]] double getInclusiveRuntimeInSeconds() const {
    return this->inclTimeInSeconds;
  }
  void setInclusiveRuntimeInSeconds(double newInclusiveTimeInSeconds) {
    this->inclTimeInSeconds = newInclusiveTimeInSeconds;
  }
  unsigned long long getNumberOfCallsWithCurrentEdges() const {
    auto v = 0ull;
    for (const auto &p : callFrom) {
      v += p.second;
    }
    return v;
  }
  unsigned long long getNumberOfCalls(CgNodePtr parentNode) { return callFrom[parentNode]; }

 private:
  unsigned long long numCalls = 0;
  double timeInSeconds = .0;
  double inclTimeInSeconds = .0;
  int threadId = 0;
  int processId = 0;
  std::unordered_map<CgNodePtr, unsigned long long> callFrom;
  std::unordered_map<CgNodePtr, double> timeFrom;
};

/**
 * This class holds data relevant to the PIRA I analyses.
 * Most notably, it offers the number of statements and the principal (dominant) runtime node
 */
class PiraOneData : public MetaData {
 public:
  static constexpr const char *key() { return "PiraOneData"; }

  void setNumberOfStatements(int numStmts) { this->numStmts = numStmts; }
  int getNumberOfStatements() const { return this->numStmts; }
  void setDominantRuntime() { this->dominantRuntime = true; }
  bool isDominantRuntime() const { return this->dominantRuntime; }
  void setComesFromCube(bool fromCube = true) { this->wasInPreviousProfile = fromCube; }
  bool comesFromCube() const { return this->wasInPreviousProfile || !this->filename.empty(); }
  bool inPreviousProfile() const { return wasInPreviousProfile; }

 private:
  bool wasInPreviousProfile = false;
  bool dominantRuntime = false;
  int numStmts = 0;
  std::string filename;
};

/**
 * This class holds data relevant to the PIRA II anslyses.
 * Most notably it encapsulates the Extra-P peformance models
 */
class PiraTwoData : public MetaData {
 public:
  static constexpr const char *key() { return "PiraTwoData"; }

  PiraTwoData(const extrapconnection::ExtrapConnector &ec) : epCon(ec) {}
  void setExtrapModelConnector(extrapconnection::ExtrapConnector epCon) { this->epCon = epCon; }
  auto &getExtrapModelConnector() { return epCon; }

 private:
  extrapconnection::ExtrapConnector epCon;
};

}  // namespace pira

#endif
