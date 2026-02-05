#pragma once

#include <flang/Semantics/symbol.h>

#include "edge.h"
#include "function.h"
#include "type.h"
#include "util.h"

using namespace Fortran::semantics;

struct trackedVar {
  Symbol* var;
  Symbol* procedure;  // procedure in which var was defined
  bool hasBeenInitialized = false;
  bool addFinalizers = false;

  trackedVar(Symbol* var, Symbol* procedure)
      : var(var), procedure(procedure), hasBeenInitialized(false), addFinalizers(false) {}
  trackedVar(Symbol* var, Symbol* procedure, bool initialized, bool addFinalizers)
      : var(var), procedure(procedure), hasBeenInitialized(initialized), addFinalizers(addFinalizers) {}
};

struct variableTracking {
  std::vector<trackedVar>& trackedVars;

  variableTracking(std::vector<trackedVar>& trackedVars) : trackedVars(trackedVars) {}

  trackedVar* getTrackedVarFromSourceName(Symbol* currentFunctionSymbol, SourceName sourceName);

  /**
   * @brief Search trackedVars for a canditate and set it as initialized.
   * Prefers local variables when (shadowed).
   *
   * @param sourceName
   */
  void handleTrackedVarAssignment(Symbol* currentFunctionSymbol, SourceName sourceName);

  /**
   * @brief Go through trackedVars vector and
   *
   * @param functions
   * @param currentFunctionSymbol
   */
  void handleTrackedVars(std::unique_ptr<edgeManager>& edgeM, std::vector<type>& types,
                         std::vector<function>& functions, Symbol* currentFunctionSymbol);

  void addTrackedVar(trackedVar var);
  void removeTrackedVars(Symbol* procedureSymbol);
};
