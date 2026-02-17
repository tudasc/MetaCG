/**
 * File: ParseTreeVisitor.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#pragma once

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
#include <type_traits>
#include <typeindex>
#include <variant>
#include <vector>

#include "Edge.h"
#include "Function.h"
#include "PotentialFinalizer.h"
#include "Type.h"
#include "Util.h"
#include "VariableTracking.h"

using namespace Fortran::parser;
using namespace Fortran::semantics;
using namespace Fortran::common;
using namespace metacg;

/**
 * @class ParseTreeVisitor
 * @brief Implements visitor methods to traverse parse tree and generate callgraph
 *
 */
class ParseTreeVisitor {
 public:
  ParseTreeVisitor(Callgraph* cg, std::string currentFileName)
      : cg(cg),
        currentFileName(currentFileName),
        edgeM(std::make_unique<edgeManager>(edges)),
        varTracking(std::make_unique<variableTracking>(trackedVars, types, functions)) {};

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
  void addEdgesForProducesAndDerivedTypes(std::vector<const type*> typeWithDerived, const Symbol* procedureSymbol);

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

  bool Pre(const MainProgram& p);
  void Post(const MainProgram&);

  bool Pre(const FunctionSubprogram&);
  void Post(const FunctionSubprogram&);
  bool Pre(const SubroutineSubprogram&);
  void Post(const SubroutineSubprogram&);

  /**
   * @brief Set hasBody field.
   *
   * @param e
   */
  void Post(const ExecutionPart& e);

  void Post(const EntryStmt& e);

  void Post(const FunctionStmt& f);
  void Post(const EndFunctionStmt&);
  void Post(const SubroutineStmt& s);
  void Post(const EndSubroutineStmt&);

  /**
   * @brief ProcedureDesignator: A procedure being called. Handles both cases a call with call statement and without.
   *
   * @param p
   */
  void Post(const ProcedureDesignator& p);

  /**
   * @brief Handle trackedVar assignment
   *
   * @param a
   */
  void Post(const AssignmentStmt& a);

  /**
   * @brief Handle trackedVar assignment through allocate statement.
   *
   * @param a
   */
  void Post(const AllocateStmt& a);

  /**
   * @brief Mostly add potential finalizers for variables that get initialized through procedure arguments.
   *
   * @param c
   */
  void Post(const Call& c);

  /**
   * @brief Mostly handles finalizers. Handles the different ways a variable can be parsed to a procedure and gets
   * initialized.
   *
   * @param t
   */
  void Post(const TypeDeclarationStmt& t);

  // The following methods are for collecting types and their procedures. See type struct and vector.

  /**
   * @brief Type definition start
   *
   * @return
   */
  bool Pre(const DerivedTypeDef&);
  /**
   * @brief Type definiiton end
   */
  void Post(const DerivedTypeDef&);

  /**
   * @brief Type stmt like type [, extends(...)] :: body (not exhaustive and not extends)
   *
   * @param t
   * @return
   */
  bool Pre(const DerivedTypeStmt& t);

  /**
   * @brief Type attributes like extends
   *
   * @param a
   */
  void Post(const TypeAttrSpec& a);

  /**
   * @brief Collect type bound procedures in derived type definitions
   *
   * @param s
   */
  void Post(const TypeBoundProcedureStmt& s);

  /**
   * @brief Collect defined operators in type definition (operator overloading)
   *
   * @param s
   */
  void Post(const TypeBoundGenericStmt& s);

  // The following methods are for collecting defined operators in interface statements

  bool Pre(const InterfaceStmt&);
  bool Pre(const EndInterfaceStmt&);
  void Post(const DefinedOperator& op);
  void Post(const ProcedureStmt& p);

  /**
   * @brief Parse operators in expressions
   *
   * @param e
   * @return
   */
  bool Pre(const Expr& e);
  /**
   * @brief Post cleanup parse operators in expressions
   *
   * @param e
   */
  void Post(const Expr& e);

  /**
   * @brief Extract additional information from use statements
   *
   * @param u
   */
  void Post(const UseStmt& u);

 private:
  Callgraph* cg;
  std::string currentFileName;
  std::unique_ptr<edgeManager> edgeM;
  std::unique_ptr<variableTracking> varTracking;

  bool inFunctionOrSubroutineSubProgram = false;
  bool inMainProgram = false;
  bool inDerivedTypeDef = false;
  bool inInterfaceStmt = false;
  bool inInterfaceStmtDefinedOperator = false;
  bool inInterfaceSpecification = false;

  std::vector<edge> edges;  // added to cg in postProcess step

  std::vector<function> functions;         // all functions
  std::vector<function> currentFunctions;  // intended as a stack. It holds the current function symbol and its dummy
                                           // args when the AST walker is in the respective function.

  std::vector<type> types;  // all types

  std::vector<std::pair<std::variant<const Symbol*, DefinedOperator::IntrinsicOperator>,
                        std::vector<const Symbol*>>>
      interfaceOperators;  // all interface operators. First is either a symbol of a DefinedOpName or
                           // IntrinsicOperator. Second is a vector procedure symbols, bound to that operator.

  std::vector<const Expr*> exprStmtWithOps;

  std::vector<trackedVar> trackedVars;  // mainly used for destructor handling

  std::vector<potentialFinalizer> potentialFinalizers;
};
