/**
 * File: ParseTreeVisitor.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#pragma once

#include "Edge.h"
#include "FortranUtil.h"
#include "Function.h"
#include "PotentialFinalizer.h"
#include "Type.h"
#include "VariableTracking.h"

#include <Callgraph.h>
#include <MCGManager.h>
#include <flang/Frontend/CompilerInstance.h>
#include <flang/Frontend/FrontendAction.h>
#include <flang/Frontend/FrontendActions.h>
#include <flang/Frontend/FrontendPluginRegistry.h>
#include <flang/Parser/dump-parse-tree.h>
#include <flang/Parser/parse-tree.h>
#include <flang/Parser/parsing.h>
#include <flang/Semantics/tools.h>
#include <io/MCGReader.h>
#include <io/MCGWriter.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/raw_ostream.h>
#include <variant>
#include <vector>

namespace metacg::cgfcollector {

/**
 * @class ParseTreeVisitor
 * @brief Implements visitor methods to traverse parse tree and generate callgraph
 *
 */
class ParseTreeVisitor {
 public:
  ParseTreeVisitor(metacg::Callgraph* cg, std::string currentFileName)
      : cg(cg),
        currentFileName(currentFileName),
        edgeM(std::make_unique<EdgeManager>(edges)),
        varTracking(std::make_unique<VariableTracking>(trackedVars, types, functions)) {};

  /**
   * @brief Collects function/subroutine statements (begin) and their dummy args.
   *
   * @tparam T
   * @param stmt
   */
  template <typename T>
  void handleFuncSubStmt(const T& stmt);

  /**
   * @brief Handles function/subroutine end statements.
   */
  void handleEndFuncSubStmt();

  /**
   * @brief Searches with typeSymbol for a type in types vector and adds edges for procedures that matches
   * procedureSymbol. And also adds edges from types that extends from typeSymbol.
   *
   * @param typeWithDerived
   * @param procedureSymbol
   */
  void addEdgesForProducesAndDerivedTypes(std::vector<const Type*> typeWithDerived,
                                          const Fortran::semantics::Symbol* procedureSymbol);

  /**
   * @brief Adds edges and potential finalizers edges to cg.
   */
  void postProcess();

  // visitor methods

  template <typename A>
  bool Pre(const A&) {
    return true;
  }
  template <typename A>
  void Post(const A&) {}

  bool Pre(const Fortran::parser::MainProgram& p);

  void Post(const Fortran::parser::MainProgram&);

  bool Pre(const Fortran::parser::FunctionSubprogram&);

  void Post(const Fortran::parser::FunctionSubprogram&);

  bool Pre(const Fortran::parser::SubroutineSubprogram&);

  void Post(const Fortran::parser::SubroutineSubprogram&);

  /**
   * @brief Set hasBody field.
   *
   * @param e
   */
  void Post(const Fortran::parser::ExecutionPart& e);

  void Post(const Fortran::parser::EntryStmt& e);

  void Post(const Fortran::parser::FunctionStmt& f);

  void Post(const Fortran::parser::EndFunctionStmt&);

  void Post(const Fortran::parser::SubroutineStmt& s);

  void Post(const Fortran::parser::EndSubroutineStmt&);

  /**
   * @brief ProcedureDesignator: A procedure being called. Handles both cases a call with call statement and without.
   *
   * @param p
   */
  void Post(const Fortran::parser::ProcedureDesignator& p);

  /**
   * @brief Handle trackedVar assignment
   *
   * @param a
   */
  void Post(const Fortran::parser::AssignmentStmt& a);

  /**
   * @brief Handle trackedVar assignment through allocate statement.
   *
   * @param a
   */
  void Post(const Fortran::parser::AllocateStmt& a);

  /**
   * @brief Mostly add potential finalizers for variables that get initialized through procedure arguments.
   *
   * @param c
   */
  void Post(const Fortran::parser::Call& c);

  /**
   * @brief Mostly handles finalizers. Handles the different ways a variable can be parsed to a procedure and gets
   * initialized.
   *
   * @param t
   */
  void Post(const Fortran::parser::TypeDeclarationStmt& t);

  // The following methods are for collecting types and their procedures. See type struct and vector.

  /**
   * @brief Type definition start
   *
   * @return
   */
  bool Pre(const Fortran::parser::DerivedTypeDef&);
  /**
   * @brief Type definiiton end
   */
  void Post(const Fortran::parser::DerivedTypeDef&);

  /**
   * @brief Type stmt like type [, extends(...)] :: body (not exhaustive and not extends)
   *
   * @param t
   * @return
   */
  bool Pre(const Fortran::parser::DerivedTypeStmt& t);

  /**
   * @brief Type attributes like extends
   *
   * @param a
   */
  void Post(const Fortran::parser::TypeAttrSpec& a);

  /**
   * @brief Collect type bound procedures in derived type definitions
   *
   * @param s
   */
  void Post(const Fortran::parser::TypeBoundProcedureStmt& s);

  /**
   * @brief Collect defined operators in type definition (operator overloading)
   *
   * @param s
   */
  void Post(const Fortran::parser::TypeBoundGenericStmt& s);

  // The following methods are for collecting defined operators in interface statements

  bool Pre(const Fortran::parser::InterfaceStmt&);

  bool Pre(const Fortran::parser::EndInterfaceStmt&);

  void Post(const Fortran::parser::DefinedOperator& op);

  void Post(const Fortran::parser::ProcedureStmt& p);

  /**
   * @brief Parse operators in expressions
   *
   * @param e
   * @return
   */
  bool Pre(const Fortran::parser::Expr& e);
  /**
   * @brief Post cleanup parse operators in expressions
   *
   * @param e
   */
  void Post(const Fortran::parser::Expr& e);

  /**
   * @brief Extract additional information from use statements
   *
   * @param u
   */
  void Post(const Fortran::parser::UseStmt& u);

 private:
  metacg::Callgraph* cg;
  std::string currentFileName;
  std::unique_ptr<EdgeManager> edgeM;
  std::unique_ptr<VariableTracking> varTracking;

  bool inFunctionOrSubroutineSubProgram = false;
  bool inMainProgram = false;
  bool inDerivedTypeDef = false;
  bool inInterfaceStmt = false;
  bool inInterfaceStmtDefinedOperator = false;
  bool inInterfaceSpecification = false;

  // added to cg in postProcess step
  std::vector<Edge> edges;

  // all functions. This vector collects all functions (+ dummy args) that were discovered during traversal of the parse
  // tree.
  std::vector<Function> functions;

  // intended as a stack. It holds the current function symbol and its dummy
  // args when the AST walker is in the respective function.
  std::vector<Function> currentFunctions;

  // all types. This vector collects all types that were discovered during traversal of the parse tree.
  std::vector<Type> types;

  // all interface operators. First is either a symbol of a DefinedOpName or
  // IntrinsicOperator. Second is a vector procedure symbols, bound to that operator.
  std::vector<
      std::pair<std::variant<const Fortran::semantics::Symbol*, Fortran::parser::DefinedOperator::IntrinsicOperator>,
                std::vector<const Fortran::semantics::Symbol*>>>
      interfaceOperators;

  std::vector<const Fortran::parser::Expr*> exprStmtWithOps;

  // mainly used for destructor handling
  std::vector<TrackedVar> trackedVars;

  std::vector<PotentialFinalizer> potentialFinalizers;
};

}  // namespace metacg::cgfcollector
