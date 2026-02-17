/**
 * File: VariableTracking.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "VariableTracking.h"

#include "FortranUtil.h"

using namespace Fortran::semantics;
using namespace metacg;

TrackedVar* VariableTracking::getTrackedVarFromSourceName(const Symbol* currentFunctionSymbol, SourceName sourceName) {
  auto anyTrackedVarIt = std::find_if(trackedVars.begin(), trackedVars.end(),
                                      [&](const TrackedVar& t) { return t.var->name() == sourceName; });
  if (anyTrackedVarIt == trackedVars.end())
    return nullptr;

  // find local variable with the same name in the current function scope (shadowed)
  auto localVarIt = std::find_if(trackedVars.begin(), trackedVars.end(), [&](const TrackedVar& t) {
    return t.var->name() == sourceName && t.procedure == currentFunctionSymbol;
  });

  // prefer local var if found
  return (localVarIt != trackedVars.end()) ? &(*localVarIt) : &(*anyTrackedVarIt);
}

void VariableTracking::handleTrackedVarAssignment(const Symbol* currentFunctionSymbol, SourceName sourceName) {
  TrackedVar* trackedVar = getTrackedVarFromSourceName(currentFunctionSymbol, sourceName);
  if (!trackedVar)
    return;

  trackedVar->hasBeenInitialized = true;

  MCGLogger::logDebug("Tracked var assigned: {} ({})", trackedVar->var->name(), fmt::ptr(trackedVar->var));
}

void VariableTracking::handleTrackedVars(const Symbol* currentFunctionSymbol, std::unique_ptr<EdgeManager>& edgeM) {
  if (mangleSymbol(currentFunctionSymbol) != "_QQmain") {
    if (!trackedVars.empty())
      MCGLogger::logDebug("Handle tracked vars for function");

    for (TrackedVar& trackedVar : trackedVars) {
      if (!trackedVar.hasBeenInitialized)
        continue;
      if (trackedVar.procedure != currentFunctionSymbol)
        continue;

      // add edge for deconstruction (finalizer)
      if (trackedVar.addFinalizers) {
        edgeM->addEdgesForFinalizers(types, currentFunctionSymbol, trackedVar.var);
      }

      // set init on dummy function args
      auto functionIt = std::find_if(functions.begin(), functions.end(),
                                     [&](const Function& f) { return f.symbol == currentFunctionSymbol; });
      if (functionIt != functions.end()) {
        auto dummyArgIt = std::find_if(functionIt->dummyArgs.begin(), functionIt->dummyArgs.end(),
                                       [&](const Function::DummyArg& d) { return d.symbol == trackedVar.var; });
        if (dummyArgIt != functionIt->dummyArgs.end()) {
          dummyArgIt->hasBeenInitialized = true;
        }
      }
    }
  }

  // cleanup trackedVars
  removeTrackedVars(currentFunctionSymbol);
}

void VariableTracking::addTrackedVar(TrackedVar var) {
  auto it = std::find_if(trackedVars.begin(), trackedVars.end(), [&](const TrackedVar& t) { return t.var == var.var; });
  if (it != trackedVars.end()) {
    // update info
    it->addFinalizers = var.addFinalizers;
    it->hasBeenInitialized = var.hasBeenInitialized;
    MCGLogger::logDebug("Update tracked variable: {} ({})", var.var->name(), fmt::ptr(var.var));
    return;
  }

  trackedVars.push_back(var);
  MCGLogger::logDebug("Add tracking for variable: {} ({})", var.var->name(), fmt::ptr(var.var));
}

void VariableTracking::removeTrackedVars(const Symbol* procedureSymbol) {
  trackedVars.erase(std::remove_if(trackedVars.begin(), trackedVars.end(),
                                   [&](const TrackedVar& t) { return t.procedure == procedureSymbol; }),
                    trackedVars.end());
}
