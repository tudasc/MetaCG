/**
 * File: VariableTracking.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#ifndef METACG_CGFCOLLECTOR_VARIABLETRACKING_H
#define METACG_CGFCOLLECTOR_VARIABLETRACKING_H

#include "Edge.h"
#include "Function.h"
#include "Type.h"

#include <flang/Semantics/symbol.h>

namespace metacg::cgfcollector {

/**
 * @class TrackedVar
 * @brief Stores information about a tracked variable.
 *
 */
struct TrackedVar {
  const Fortran::semantics::Symbol* var;

  // procedure in which var was defined
  const Fortran::semantics::Symbol* procedure;

  bool hasBeenInitialized = false;
  bool addFinalizers = false;

  TrackedVar(const Fortran::semantics::Symbol* var, const Fortran::semantics::Symbol* procedure)
      : var(var), procedure(procedure), hasBeenInitialized(false), addFinalizers(false) {}
  TrackedVar(const Fortran::semantics::Symbol* var, const Fortran::semantics::Symbol* procedure, bool initialized,
             bool addFinalizers)
      : var(var), procedure(procedure), hasBeenInitialized(initialized), addFinalizers(addFinalizers) {}
};

/**
 * @class VariableTracking
 * @brief Handle tracking of variable initializations and finalizations at the end of the function scope.
 *
 * NOTE: Variables cannot be reliably tracked across multiple procedure calls. If a variable is passed into one
 * procedure and then forwarded to another, it is no longer suffientily tracked and remains uninitialized in
 * `trackedVars`.
 */
struct VariableTracking {
 public:
  VariableTracking(
      std::vector<TrackedVar>& trackedVars, std::vector<Type>& types, std::vector<Function>& functions,
      std::unordered_map<const Fortran::semantics::Symbol*, std::vector<const Fortran::semantics::Symbol*>>& finalizers,
      bool underscoring)
      : trackedVars(trackedVars),
        types(types),
        functions(functions),
        finalizers(finalizers),
        underscoring(underscoring) {}

  /**
   * @brief Search trackedVars for a canditate by source name. Also taking the current function scope into account.
   *
   * @param currentFunctionSymbol
   * @param sourceName
   * @return trackedVar* or nullptr if not found
   */
  [[nodiscard]] TrackedVar* getTrackedVarFromSourceName(const Fortran::semantics::Symbol* currentFunctionSymbol,
                                                        Fortran::semantics::SourceName sourceName);

  /**
   * @brief Search trackedVars for a canditate and set it as initialized.
   *
   * @param currentFunctionSymbol
   * @param sourceName
   */
  void handleTrackedVarAssignment(const Fortran::semantics::Symbol* currentFunctionSymbol,
                                  Fortran::semantics::SourceName sourceName);

  /**
   * @brief Is called at the end of a function/subroutine end statement. It checks trackedVars for any initialized
   * variables and adds edges. Currently only adds finalizer edges.
   *
   * @param currentFunctionSymbol
   * @param edgeM TODO: remove dep
   */
  void handleTrackedVars(const Fortran::semantics::Symbol* currentFunctionSymbol, std::unique_ptr<EdgeManager>& edgeM);

  /**
   * @brief Register a variable for tracking.
   *
   * @param var
   */
  void addTrackedVar(TrackedVar var);

  /**
   * @brief Remove all tracked variables that are not needed anymore after the function/subroutine end statement. Should
   * be called at the end of function/subroutine processing.
   *
   * @param procedureSymbol
   */
  void removeTrackedVars(const Fortran::semantics::Symbol* procedureSymbol);

 private:
  std::vector<TrackedVar>& trackedVars;
  std::vector<Type>& types;
  std::vector<Function>& functions;
  std::unordered_map<const Fortran::semantics::Symbol*, std::vector<const Fortran::semantics::Symbol*>>& finalizers;
  bool underscoring;
};

}  // namespace metacg::cgfcollector

#endif  // METACG_CGFCOLLECTOR_VARIABLETRACKING_H
