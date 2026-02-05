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

#include "util.h"

using namespace Fortran::parser;
using namespace Fortran::semantics;
using namespace Fortran::common;
using namespace metacg;

using edge = std::pair<std::string, std::string>;  // (caller, callee)

struct type {
  Symbol* type;
  Symbol* extendsFrom;
  std::vector<std::pair<Symbol*, Symbol*>> procedures;                            // name(symbol) => optname(symbol)
  std::vector<std::pair<DefinedOperator::IntrinsicOperator, Symbol*>> operators;  // operator => name(symbol)
};

struct trackedVar {
  Symbol* var;
  Symbol* procedure;  // procedure in which var was defined
  bool hasBeenInitialized = false;
  bool addFinalizers = false;
};

struct function {
  struct dummyArg {
    Symbol* symbol;
    bool hasBeenInitialized = false;
  };

  Symbol* symbol;  // function symbol
  std::vector<dummyArg> dummyArgs;
};

struct potentialFinalizer {
  std::size_t argPos;
  std::string procedureCalled;
  std::vector<edge> finalizerEdges;
};

class ParseTreeVisitor {
 public:
  ParseTreeVisitor(Callgraph* cg, std::string currentFileName) : cg(cg), currentFileName(currentFileName) {};

  template <typename T>
  void handleFuncSubStmt(const T& stmt);
  void handleEndFuncSubStmt();

  /**
   * @brief Searches the types vector for given typeSymbol and returns pointers to vectors of type with derived types.
   *
   * @param typeSymbol symbol to search for
   * @return vector with type and all derived types
   */
  std::vector<const type*> findTypeWithDerivedTypes(const Symbol* typeSymbol);

  /**
   * @brief Searches with typeSymbol for a type in types vector and adds edges for procedures that matches
   * procedureSymbol. And also adds edges from types that extends from typeSymbol.
   *
   * @param typeWithDerived
   * @param procedureSymbol
   */
  void addEdgesForProducesAndDerivedTypes(std::vector<const type*> typeWithDerived, const Symbol* procedureSymbol);

  void addEdgesForFinalizers(const Symbol* typeSymbol);
  void addEdgesForFinalizers(std::vector<edge>* edges, const Symbol* typeSymbol);
  std::vector<std::pair<Symbol*, const Symbol*>> getEdgesForFinalizers(const Symbol* typeSymbol);

  trackedVar* getTrackedVarFromSourceName(SourceName sourceName);

  /**
   * @brief Search trackedVars for a canditate and set it as initialized.
   * Prefers local variables when (shadowed).
   *
   * @param sourceName
   */
  void handleTrackedVarAssignment(SourceName sourceName);

  void addTrackedVar(trackedVar var);
  void removeTrackedVars(Symbol* procedureSymbol);

  void handleTrackedVars();

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

  void Post(const ExecutionPart& e);

  void Post(const EntryStmt& e);

  void Post(const FunctionStmt& f);
  void Post(const EndFunctionStmt&);
  void Post(const SubroutineStmt& s);
  void Post(const EndSubroutineStmt&);

  void Post(const ProcedureDesignator& p);

  void Post(const AssignmentStmt& a);
  void Post(const AllocateStmt& a);
  void Post(const Call& c);

  /**
   * @brief Handle finalizers (destructors ). TODO: test i definitely missed some edges cases.
   *
   * @param t
   */
  void Post(const TypeDeclarationStmt& t);

  // The following methods are for collecting types and their procedures. see type struct and vector.

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
  std::vector<edge> edges;
  std::string currentFileName;

  bool inFunctionOrSubroutineSubProgram = false;
  bool inMainProgram = false;
  bool inDerivedTypeDef = false;
  bool inInterfaceStmt = false;
  bool inInterfaceStmtDefinedOperator = false;
  bool inInterfaceSpecification = false;

  std::vector<Symbol*> functionSymbols;  // intended as a stack. It holds the current procedure symbol when the AST
                                         // walker is in the respective procedure.
  std::vector<std::vector<const Name*>> functionDummyArgs;  // same idea, but for dummy args

  std::vector<type> types;  // all types

  std::vector<std::pair<std::variant<Symbol*, DefinedOperator::IntrinsicOperator>,
                        std::vector<Symbol*>>>
      interfaceOperators;  // all interface operators. First is either a symbol of a DefinedOpName or
                           // IntrinsicOperator. Second is a vector procedure symbols, bound to that operator.

  std::vector<const Expr*> exprStmtWithOps;

  // mainly used for destructor handling
  std::vector<trackedVar> trackedVars;

  std::vector<function> functions;  // all functions

  std::vector<potentialFinalizer> potentialFinalizers;
};
