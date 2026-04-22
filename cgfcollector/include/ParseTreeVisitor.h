/**
 * File: ParseTreeVisitor.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#ifndef METACG_CGFCOLLECTOR_PARSETREEVISITOR_H
#define METACG_CGFCOLLECTOR_PARSETREEVISITOR_H

#include "Edge.h"
#include "FortranUtil.h"
#include "Function.h"
#include "PotentialFinalizer.h"
#include "Type.h"
#include "VariableTracking.h"

#include <flang/Frontend/CompilerInstance.h>
#include <flang/Frontend/FrontendAction.h>
#include <flang/Frontend/FrontendActions.h>
#include <flang/Frontend/FrontendPluginRegistry.h>
#include <flang/Parser/dump-parse-tree.h>
#include <flang/Parser/parse-tree.h>
#include <flang/Parser/parsing.h>
#include <flang/Semantics/tools.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/raw_ostream.h>
#include <metacg/Callgraph.h>
#include <metacg/MCGManager.h>
#include <metacg/io/MCGReader.h>
#include <metacg/io/MCGWriter.h>
#include <variant>
#include <vector>

namespace metacg::cgfcollector {

struct pair_hash {
  template <typename T1, typename T2>
  std::size_t operator()(const std::pair<T1, T2>& p) const noexcept {
    std::size_t h1 = std::hash<T1>{}(p.first);
    std::size_t h2 = std::hash<T2>{}(p.second);
    return h1 ^ (h2 << 1);
  }
};

/**
 * @class ParseTreeVisitor
 * @brief Implements visitor methods to traverse parse tree and generate call graph.
 * Intended to be used with `Fortran::parser::Walk()`
 *
 * The parse tree is composed of different node types. Each node type has a corresponding visitor methods in this class.
 * The visitor methods are called `Pre` and `Post` methods. The `Pre` method is called before visiting the children of
 * the node, and the Post method is called after visiting the children of the node. All possible node types and there
 * relations are defined through a grammar. Can be found here: https://flang.llvm.org/docs/f2018-grammar.html
 */
class ParseTreeVisitor {
 public:
  ParseTreeVisitor(metacg::Callgraph* cg, std::string currentFileName, bool underscoring, bool includeInstrinsics)
      : cg(cg),
        currentFileName(currentFileName),
        underscoring(underscoring),
        includeInstrinsics(includeInstrinsics),
        edgeM(std::make_unique<EdgeManager>(edges, underscoring)),
        varTracking(std::make_unique<VariableTracking>(trackedVars, types, functions, finalizers, underscoring)) {};

  /**
   * @brief Add dummy args to current function in `currentFunctions` and `functions`. Also initiates variable tracking.
   *
   * @tparam Iterable
   * @tparam Extractor
   * @param items List of dummy args
   * @param extract Function to extract dummy arg to `Symbol`
   */
  template <typename Iterable, typename Extractor>
  void handleDummyArgs(const Iterable& items, Extractor extract);

  /**
   * @brief Collects function/subroutine statements (begin) and their dummy args.
   *
   * This method populates `currentFunctions` and `functions` vectors. It also adds a node for the procedure to the call
   * graph. Also this method should only be called from a `FunctionStmt` and `SubroutineStmt` visitor methods. It is
   * implemented as a template to avoid code duplication.
   *
   * @tparam T
   * @param stmt
   */
  template <typename T>
  void handleFuncSubStmt(const T& stmt);

  /**
   * @brief Handles function/subroutine end statements.
   *
   * At the end of a function, we handle the tracked variables and maintain `currentFunctions`.
   */
  void handleEndFuncSubStmt();

  /**
   * @brief For a list of `Type` add edges, from the current procedure, to procedures that match the `procedureSymbol`.
   * The comparison is done by source name. For each `Type` the list contains, is also needs to contain all types that
   * extend or are extended from.
   *
   * This is used for handling polymorphic calls. This adds all possible edges, that could be called at runtime through
   * polymorphism.
   *
   * The `findTypeWithDerivedTypes` function can be used, to get the list of `Type` with the required prerequisites.
   *
   * @param typeWithDerived
   * @param procedureSymbol
   */
  void addEdgesForProceduresAndDerivedTypes(std::vector<const Type*> typeWithDerived,
                                            const Fortran::semantics::Symbol* procedureSymbol);

  /**
   * @brief Add uniquefied edges and potential finalizers edges to the call graph.
   *
   * Needs to be called after the parse tree traversal.
   *
   * Edges are uniquefied because the graph lib warns about duplicate edges. This leads to a lot of warns being printed
   * on screen, as we potentially collect a edge many times.
   */
  void postProcess();

  // visitor methods

  template <typename A>
  bool Pre(const A&) {
    return true;
  }
  template <typename A>
  void Post(const A&) {}

  /**
   * @brief Pre visitor of the main procedure of the Fortran program.
   *
   * @param p
   * @return
   */
  bool Pre(const Fortran::parser::MainProgram& p);

  /**
   * @brief Post visitor of the main procedure.
   */
  void Post(const Fortran::parser::MainProgram&);

  /**
   * @brief Bookkeeping that we are in a function.
   *
   * @return
   */
  bool Pre(const Fortran::parser::FunctionSubprogram&);

  /**
   * @brief Bookkeeping that we are in a function.
   */
  void Post(const Fortran::parser::FunctionSubprogram&);

  /**
   * @brief Bookkeeping that we are in a subroutine.
   *
   * @return
   */
  bool Pre(const Fortran::parser::SubroutineSubprogram&);

  /**
   * @brief Bookkeeping that we are in a subroutine.
   */
  void Post(const Fortran::parser::SubroutineSubprogram&);

  /**
   * @brief Set `hasBody` field.
   *
   * @param e
   */
  void Post(const Fortran::parser::ExecutionPart& e);

  /**
   * @brief Handle the entry statement like a normal procedure.
   *
   * @param e
   */
  void Post(const Fortran::parser::EntryStmt& e);

  /**
   * @brief Function entry: Call `handleFuncSubStmt`, and collect function arguments for `functions` vector.
   *
   * @param f
   */
  void Post(const Fortran::parser::FunctionStmt& f);

  /**
   * @brief calls `handleEndFuncSubStmt`
   */
  void Post(const Fortran::parser::EndFunctionStmt&);

  /**
   * @brief Function entry: Call `handleFuncSubStmt`, and collect subroutine arguments for `functions` vector.
   *
   * @param f
   */
  void Post(const Fortran::parser::SubroutineStmt& s);

  /**
   * @brief calls `handleEndFuncSubStmt`
   */
  void Post(const Fortran::parser::EndSubroutineStmt&);

  /**
   * @brief `ProcedureDesignator`: A procedure being called. Handles both cases: function and subroutine calls.
   *
   * @param p
   */
  void Post(const Fortran::parser::ProcedureDesignator& p);

  /**
   * @brief Handle `trackedVar` assignment
   *
   * @param a
   */
  void Post(const Fortran::parser::AssignmentStmt& a);

  /**
   * @brief Handle `trackedVar` assignment through allocate statement.
   *
   * @param a
   */
  void Post(const Fortran::parser::AllocateStmt& a);

  /**
   * @brief Mostly add potential finalizers for variables that get initialized through procedure arguments.
   *
   * It iterations through the arguments of the call statement and adds potential finalizers for all arguments that a
   * registered as a tracked variable.
   *
   * It also handles the case where a variable is initialized through the `move_alloc` intrinsic.
   *
   * @param c
   */
  void Post(const Fortran::parser::Call& c);

  /**
   * @brief A `TypeDeclarationStmt` is an argument in the procedure definition. This visitor mostly handles finalizers.
   * Handles the different ways a variable can be parsed to a procedure and gets initialized.
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
   * @brief Type definition end
   */
  void Post(const Fortran::parser::DerivedTypeDef&);

  /**
   * @brief Type statement like type [, extends(...)] :: body (not exhaustive and not extends)
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
   * @brief Collect type-bound procedures in derived type definitions
   *
   * @param s
   */
  void Post(const Fortran::parser::TypeBoundProcedureStmt& s);

  /**
   * @brief Collect type-bound operators (operator overloading)
   *
   * @param s
   */
  void Post(const Fortran::parser::TypeBoundGenericStmt& s);

  // The following methods are for collecting defined operators in interface statements

  /**
   * @brief Bookkeeping: In interface statement
   *
   * @return
   */
  bool Pre(const Fortran::parser::InterfaceStmt&);

  /**
   * @brief Bookkeeping: Leaving interface statement
   *
   * @return
   */
  bool Pre(const Fortran::parser::EndInterfaceStmt&);

  /**
   * @brief Collect interface operators and store in `interfaceOperators`. The second element in `interfaceOperators` is
   * a list of procedure symbols that are specified for the operator. These get collected in the `ProcedureStmt`
   * visitor.
   *
   * @param op
   */
  void Post(const Fortran::parser::DefinedOperator& op);

  /**
   * @brief A `ProcedureStmt` references one or more procedures. We use it to collect procedures specified for an
   * interface operator.
   *
   * @param p
   */
  void Post(const Fortran::parser::ProcedureStmt& p);

  /**
   * @brief Handle operator overloading. Uses `interfaceOperators` and `types` vectors to extract the procedures getting
   * called when evaluation an expression with operators. For example, `(.NOT. 324 < 2) .EQV. .true.` has the operators
   * `.NOT.`, `<`, and `.EQV.`. If these operators are overloaded, we need to find the procedures that are called for
   * this expression. To go this we first go to the lowest operator in the expression (`<` in the example) and then go
   * up the expression tree. For each operator we check if it is overloaded through an interface operator or a
   * type-bound operator. If it is, we add edges to the procedures that are specified for the operator. For the
   * type-bound operators we also need to check for any polymorphic calls.
   *
   * There are two ways to define operators in Fortran: through interface statements and through type definitions.
   *
   * @param e
   * @return
   */
  bool Pre(const Fortran::parser::Expr& e);

  /**
   * @brief Maintain `exprStmtWithOps` for `Pre` visitor of `Expr`.
   *
   * @param e
   */
  void Post(const Fortran::parser::Expr& e);

  /**
   * @brief Extract additional information from use statements. This includes derived types, interface operators, and
   * procedures.
   *
   * @param u
   */
  void Post(const Fortran::parser::UseStmt& u);

  /**
   * @brief Handle StmtFunctionStmt like normal functions
   *
   * @param u
   */
  bool Pre(const Fortran::parser::StmtFunctionStmt& s);

  /**
   * @brief Handle StmtFunctionStmt like normal functions
   *
   * @param u
   */
  void Post(const Fortran::parser::StmtFunctionStmt& s);

 private:
  metacg::Callgraph* cg;
  std::string currentFileName;
  bool underscoring;
  bool includeInstrinsics;

  bool inFunctionOrSubroutineSubProgram = false;
  bool inMainProgram = false;
  bool inDerivedTypeDef = false;
  bool inInterfaceStmt = false;
  bool inInterfaceStmtDefinedOperator = false;
  bool inInterfaceSpecification = false;

  std::unique_ptr<EdgeManager> edgeM;

  std::unique_ptr<VariableTracking> varTracking;

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

  // Maps each (type, binding name) pair to the set of procedures that override it,
  // allowing lookup of all overriding implementations via polymorphism.
  // {type, procBinding name} -> {overriding procedures}
  std::unordered_map<std::pair<const Fortran::semantics::Symbol*, std::string>,
                     std::vector<const Fortran::semantics::Symbol*>, pair_hash>
      procedureOverwrites;

  // type -> finalizer procedures
  std::unordered_map<const Fortran::semantics::Symbol*, std::vector<const Fortran::semantics::Symbol*>> finalizers;

  // {type, operator} -> {binding names}
  std::unordered_map<std::pair<const Fortran::semantics::Symbol*, std::string>, std::vector<std::string>, pair_hash>
      typeOperators;
};

}  // namespace metacg::cgfcollector

#endif  // METACG_CGFCOLLECTOR_PARSETREEVISITOR_H
