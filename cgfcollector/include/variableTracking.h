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
 public:
  variableTracking(std::vector<trackedVar>& trackedVars, std::vector<type>& types, std::vector<function>& functions)
      : trackedVars(trackedVars), types(types), functions(functions) {}

  /**
   * @brief Search trackedVars for a canditate by sourceName. Also taking the current function scope into account.
   *
   * @param currentFunctionSymbol
   * @param sourceName
   * @return trackedVar* or nullptr if not found
   */
  trackedVar* getTrackedVarFromSourceName(Symbol* currentFunctionSymbol, SourceName sourceName);

  /**
   * @brief Search trackedVars for a canditate and set it as initialized.
   *
   * @param currentFunctionSymbol
   * @param sourceName
   */
  void handleTrackedVarAssignment(Symbol* currentFunctionSymbol, SourceName sourceName);

  /**
   * @brief Is called at the end of a function/subroutine end statement. It checks trackedVars for any initialized
   * variables and adds edges. Currently only adds finalizer edges.
   *
   * @param currentFunctionSymbol
   * @param edgeM TODO: remove dep
   */
  void handleTrackedVars(Symbol* currentFunctionSymbol, std::unique_ptr<edgeManager>& edgeM);

  /**
   * @brief Register a variable for tracking.
   *
   * @param var
   */
  void addTrackedVar(trackedVar var);

  /**
   * @brief Remove all tracked variables that are not needed anymore after the function/subroutine end statement. Should
   * be called at the end of function/subroutine processing.
   *
   * @param procedureSymbol
   */
  void removeTrackedVars(Symbol* procedureSymbol);

 private:
  std::vector<trackedVar>& trackedVars;
  std::vector<type>& types;
  std::vector<function>& functions;
};
