/**
 * File: VariableTracking.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#pragma once

#include <flang/Semantics/symbol.h>

#include "Edge.h"
#include "Function.h"
#include "Type.h"
#include "Util.h"

using namespace Fortran::semantics;

struct trackedVar {
  const Symbol* var;
  const Symbol* procedure;  // procedure in which var was defined
  bool hasBeenInitialized = false;
  bool addFinalizers = false;

  trackedVar(const Symbol* var, const Symbol* procedure)
      : var(var), procedure(procedure), hasBeenInitialized(false), addFinalizers(false) {}
  trackedVar(const Symbol* var, const Symbol* procedure, bool initialized, bool addFinalizers)
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
  trackedVar* getTrackedVarFromSourceName(const Symbol* currentFunctionSymbol, SourceName sourceName);

  /**
   * @brief Search trackedVars for a canditate and set it as initialized.
   *
   * @param currentFunctionSymbol
   * @param sourceName
   */
  void handleTrackedVarAssignment(const Symbol* currentFunctionSymbol, SourceName sourceName);

  /**
   * @brief Is called at the end of a function/subroutine end statement. It checks trackedVars for any initialized
   * variables and adds edges. Currently only adds finalizer edges.
   *
   * @param currentFunctionSymbol
   * @param edgeM TODO: remove dep
   */
  void handleTrackedVars(const Symbol* currentFunctionSymbol, std::unique_ptr<edgeManager>& edgeM);

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
  void removeTrackedVars(const Symbol* procedureSymbol);

 private:
  std::vector<trackedVar>& trackedVars;
  std::vector<type>& types;
  std::vector<function>& functions;
};
