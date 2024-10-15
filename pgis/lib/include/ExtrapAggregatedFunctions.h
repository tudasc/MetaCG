/**
 * File: ExtrapAggregatedFunctions.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#ifndef PGIS_EXTRAPAGGREGATEDFUNCTIONS_H
#define PGIS_EXTRAPAGGREGATEDFUNCTIONS_H

#include <utility>

/**
 *
 */
class AggregatedFunction : public EXTRAP::Function {
 public:
  explicit AggregatedFunction(std::vector<EXTRAP::Model*> submodels) : submodels(std::move(submodels)) {}

 protected:
  std::vector<EXTRAP::Model*> submodels;
  std::string buildString(const EXTRAP::ParameterList& parameterNames, std::string&& prefix, std::string&& seperator,
                          std::string&& postfix) const {
    std::stringstream ss;
    ss << prefix;
    for (size_t i = 0; i < submodels.size(); i++) {
      ss << submodels.at(i)->getModelFunction()->getAsString(parameterNames);
      if (i < submodels.size() - 1) {
        ss << seperator;
      }
    }
    ss << postfix;
    return ss.str();
  }

  EXTRAP::Value sum(const EXTRAP::ParameterValueList& parameterValues) const {
    EXTRAP::Value sum = 0.0;
    for (auto model : submodels) {
      EXTRAP::Value v = model->getModelFunction()->evaluate(parameterValues);
      assert(v >= 0.0);
      sum += v;
    }
    return sum;
  }

  EXTRAP::Value max(const EXTRAP::ParameterValueList& parameterValues) const {
    EXTRAP::Value max = 0.0;
    for (auto model : submodels) {
      EXTRAP::Value local = model->getModelFunction()->evaluate(parameterValues);
      assert(local >= 0.0);
      if (local > max) {
        max = local;
      }
    }
    return max;
  }
};

/**
 * This aggregation strategy always uses the first available submodel and ignores all others.
 */
class FirstModelFunction : public AggregatedFunction {
 public:
  FirstModelFunction(std::vector<EXTRAP::Model*> submodels) : AggregatedFunction(submodels) {}

  EXTRAP::Value evaluate(const EXTRAP::ParameterValueList& parameterValues) const override {
    return submodels.front()->getModelFunction()->evaluate(parameterValues);
  }

  std::string getAsString(const EXTRAP::ParameterList& parameterNames) const override {
    return submodels.front()->getModelFunction()->getAsString(parameterNames);
  }
};

/**
 * This class represents the sum function of a set of models.
 */
class SumFunction : public AggregatedFunction {
 public:
  SumFunction(std::vector<EXTRAP::Model*> submodels) : AggregatedFunction(submodels) {}

  EXTRAP::Value evaluate(const EXTRAP::ParameterValueList& parameterValues) const override {
    return sum(parameterValues);
  }

  std::string getAsString(const EXTRAP::ParameterList& parameterNames) const override {
    return buildString(parameterNames, "", " + ", "");
  }
};

/**
 * This class represents the average function of a set of models.
 */
class AvgFunction : public AggregatedFunction {
 public:
  AvgFunction(std::vector<EXTRAP::Model*> submodels) : AggregatedFunction(submodels) {}

  EXTRAP::Value evaluate(const EXTRAP::ParameterValueList& parameterValues) const override {
    return sum(parameterValues) / (double)submodels.size();
  }

  std::string getAsString(const EXTRAP::ParameterList& parameterNames) const override {
    return buildString(parameterNames, "AVG(", ", ", ")");
  }
};

/**
 * This class represents the average function of a set of models.
 */
class MaxFunction : public AggregatedFunction {
 public:
  MaxFunction(std::vector<EXTRAP::Model*> submodels) : AggregatedFunction(submodels) {}

  EXTRAP::Value evaluate(const EXTRAP::ParameterValueList& parameterValues) const override {
    return max(parameterValues);
  }

  std::string getAsString(const EXTRAP::ParameterList& parameterNames) const override {
    return buildString(parameterNames, "MAX(", ", ", ")");
  }
};

#endif  // PGIS_EXTRAPAGGREGATEDFUNCTIONS_H
