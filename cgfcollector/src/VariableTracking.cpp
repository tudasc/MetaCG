/**
 * File: VariableTracking.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "VariableTracking.h"
#include "FortranUtil.h"

using namespace Fortran::semantics;
using namespace metacg;

namespace metacg::cgfcollector {

TrackedVar* VariableTracking::getTrackedVarFromSourceName(const Symbol* currentProcedureSymbol, SourceName sourceName) {
  auto anyTrackedVarIt = std::find_if(trackedVars.begin(), trackedVars.end(),
                                      [&](const TrackedVar& t) { return t.var->name() == sourceName; });
  if (anyTrackedVarIt == trackedVars.end())
    return nullptr;

  // find local variable with the same name in the current procedure scope (shadowed)
  auto localVarIt = std::find_if(trackedVars.begin(), trackedVars.end(), [&](const TrackedVar& t) {
    return t.var->name() == sourceName && compareSymbols(t.procedure, currentProcedureSymbol);
  });

  // prefer local var if found
  return (localVarIt != trackedVars.end()) ? &(*localVarIt) : &(*anyTrackedVarIt);
}

void VariableTracking::handleTrackedVarAssignment(const Symbol* currentProcedureSymbol, SourceName sourceName) {
  TrackedVar* trackedVar = getTrackedVarFromSourceName(currentProcedureSymbol, sourceName);
  if (!trackedVar)
    return;

  trackedVar->hasBeenInitialized = true;

  MCGLogger::logDebug("Tracked var assigned: {} ({}) ({})", trackedVar->var->name(), getDetailsName(trackedVar->var),
                      fmt::ptr(trackedVar->var));
}

void VariableTracking::handleTrackedVars(const Symbol* currentProcedureSymbol, std::unique_ptr<EdgeManager>& edgeM) {
  // the fortran standard does not require finalizers for variables in the main program. So we skip it.
  // NOTE: Flang does call them.
  if (mangleSymbol(currentProcedureSymbol, underscoring) == "_QQmain") {
    return;
  }

  if (!trackedVars.empty()) {
    MCGLogger::logDebug("Handle tracked vars for procedure {} ({}) ({})",
                        mangleSymbol(currentProcedureSymbol, underscoring), getDetailsName(currentProcedureSymbol),
                        fmt::ptr(currentProcedureSymbol));
  }

  for (TrackedVar& trackedVar : trackedVars) {
    MCGLogger::logDebug("  Tracked var: {} ({}) ({}) - initialized: {}, addFinalizers: {}", trackedVar.var->name(),
                        getDetailsName(trackedVar.var), fmt::ptr(trackedVar.var), trackedVar.hasBeenInitialized,
                        trackedVar.addFinalizers);

    if (!trackedVar.hasBeenInitialized)
      continue;
    if (trackedVar.procedure != currentProcedureSymbol)
      continue;

    // add edge for deconstruction (finalizer)
    if (trackedVar.addFinalizers) {
      edgeM->addEdgesForFinalizers(types, finalizers, currentProcedureSymbol, trackedVar.var);
    }

    // set init on dummy procedure args
    auto procedureIt = std::find_if(procedures.begin(), procedures.end(), [&](const Procedure& f) {
      return compareSymbols(f.symbol, currentProcedureSymbol);
    });
    if (procedureIt != procedures.end()) {
      auto dummyArgIt =
          std::find_if(procedureIt->dummyArgs.begin(), procedureIt->dummyArgs.end(),
                       [&](const Procedure::DummyArg& d) { return compareSymbols(d.symbol, trackedVar.var); });
      if (dummyArgIt != procedureIt->dummyArgs.end()) {
        dummyArgIt->hasBeenInitialized = true;
      }
    }
  }

  // cleanup trackedVars
  removeTrackedVars(currentProcedureSymbol);
}

void VariableTracking::addTrackedVar(TrackedVar var) {
  auto it = std::find_if(trackedVars.begin(), trackedVars.end(),
                         [&](const TrackedVar& t) { return compareSymbols(t.var, var.var); });
  if (it != trackedVars.end()) {
    // update info
    it->addFinalizers = var.addFinalizers;
    it->hasBeenInitialized = var.hasBeenInitialized;
    MCGLogger::logDebug("Update tracked variable: {} ({}) ({})", var.var->name(), getDetailsName(var.var),
                        fmt::ptr(var.var));
    return;
  }

  trackedVars.push_back(var);
  MCGLogger::logDebug("Add tracking for variable: {} ({}) ({})", var.var->name(), getDetailsName(var.var),
                      fmt::ptr(var.var));
}

void VariableTracking::removeTrackedVars(const Symbol* procedureSymbol) {
  auto newEnd = std::remove_if(trackedVars.begin(), trackedVars.end(), [&](const TrackedVar& t) {
    if (compareSymbols(t.procedure, procedureSymbol)) {
      MCGLogger::logDebug("Removing tracked variable: {} ({}) ({}) for procedure: {} ({}) ({})", t.var->name(),
                          getDetailsName(t.var), fmt::ptr(t.var), mangleSymbol(procedureSymbol, underscoring),
                          getDetailsName(procedureSymbol), fmt::ptr(procedureSymbol));
      return true;
    }
    return false;
  });

  trackedVars.erase(newEnd, trackedVars.end());
}

}  // namespace metacg::cgfcollector
