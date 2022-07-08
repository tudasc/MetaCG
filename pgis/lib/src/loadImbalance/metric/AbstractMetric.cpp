/**
 * File: AbstractMetric.cpp
 * License: Part of the metacg project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "loadImbalance/metric/AbstractMetric.h"
#include "CgNodeMetaData.h"

#include <cmath>
#include <limits>
#include <map>
#include <utility>

#define DEBUG 1

LoadImbalance::AbstractMetric::AbstractMetric()
    : node(nullptr), max(.0), min(std::numeric_limits<double>::max()), mean(.0), stdev(.0), count(.0) {}

/**
 * Calculates the standard deviation of a non-empty vector
 * see
 *  - https://stackoverflow.com/a/7616783/3473012
 */
double calcStddev(std::vector<double> &v) {
  assert(v.size() > 0 &&
         "AbstractMetric.cpp: calcStddev: Can only calculate the standard deviation of a non-empty vector.");

  double sum = std::accumulate(v.begin(), v.end(), 0.0);
  double mean = sum / v.size();

  std::vector<double> diff(v.size());
  std::transform(v.begin(), v.end(), diff.begin(), std::bind2nd(std::minus<double>(), mean));
  double sqSum = std::inner_product(diff.begin(), diff.end(), diff.begin(), 0.0);
  double stdev = std::sqrt(sqSum / (v.size()));

  return stdev;
}

void LoadImbalance::AbstractMetric::calcIndicators(std::ostringstream &debugString) {
  const auto n = this->node;

  // associate execution times to their processing unit (process, thread) and accumulate if necessary
  std::map<std::pair<int, int>, double> timesTable;
  if (!n->get<pira::BaseProfileData>()->getCgLocation().empty()) {  // profile information present
    for (const auto loc : n->get<pira::BaseProfileData>()->getCgLocation()) {
      const double localTime = loc.getInclusiveTime();

      // skip empty locations
      if (localTime > 0) {
        auto procUnit = std::make_pair<int, int>(loc.getProcId(), loc.getThreadId());

        // test whether processing unit is already known
        if (timesTable.find(procUnit) != timesTable.end()) {
          timesTable[procUnit] = timesTable[procUnit] + localTime;
        } else {
          timesTable[procUnit] = localTime;
        }
      }
    }
  }

  std::vector<double> times;
  double sum = 0.;
  this->count = 0;

  debugString << " [";
  for (auto kv : timesTable) {
    // extract times from map
    times.push_back(kv.second);
    sum += kv.second;
    this->count++;

    debugString << kv.second << " ";
  }
  debugString << "]";

  this->max = 0.;
  this->min = INFINITY;
  this->mean = 0;
  this->stdev = 0;

  if (count > 0) {
    this->mean = sum / count;

    this->stdev = count > 1 ? calcStddev(times) : 0.;

    this->max = *std::max_element(times.begin(), times.end());
    this->min = *std::min_element(times.begin(), times.end());
  }
}

void LoadImbalance::AbstractMetric::setNode(CgNodePtr newNode, std::ostringstream &debugString) {
  this->node = newNode;

  this->calcIndicators(debugString);
}

void LoadImbalance::AbstractMetric::setNode(CgNodePtr newNode) {
  std::ostringstream dummy;

  this->setNode(newNode, dummy);
}
