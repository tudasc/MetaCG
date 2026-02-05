#pragma once

#include "headers.h"

#include "AL.h"

using namespace Fortran::parser;
using namespace Fortran::semantics;
using namespace Fortran::common;
using Fortran::lower::mangle::mangleName;

typedef struct type {
  Symbol* type;
  Symbol* extendsFrom;
  std::vector<std::pair<Symbol*, Symbol*>> procedures;  // name(symbol) => optname(symbol)
  std::vector<std::pair<const DefinedOperator::IntrinsicOperator*, Symbol*>> operators;  // operator => name(symbol)
} type_t;

typedef struct trackedVar {
  Symbol* var;
  Symbol* procedure;  // procedure in which var was defined
  bool hasBeenInitialized = false;
} trackedVar_t;

class ParseTreeVisitor {
 public:
  ParseTreeVisitor(metacg::Callgraph* cg, std::string currentFileName) : cg(cg), currentFileName(currentFileName) {};

  std::vector<std::pair<std::string, std::string>> getEdges() const { return edges; }

  template <typename T>
  void handleFuncSubStmt(const T& stmt);
  void handleEndFuncSubStmt();

  void handleTrackedVars();

  // searches the types vector for given typeSymbol and returns pointers to vectors of type with derived types.
  std::vector<type_t> find_type_with_derived_types(const Symbol* typeSymbol);

  // this function searches with typeSymbol for a type in types vector and adds edges for procedures that matches
  // procedureSymbol. And also adds edges from types that extends from typeSymbol.
  void add_edges_for_produces_and_derived_types(std::vector<type_t> typeWithDerived, const Symbol* procedureSymbol);

  void add_edges_for_finalizers(const Symbol* typeSymbol);

  template <typename Variant, typename... Ts>
  bool holds_any_of(const Variant& v) {
    return (std::holds_alternative<Ts>(v) || ...);
  }

  bool isOperator(const Expr* e);

  bool compare_expr_IntrinsicOperator(const Expr* expr, const DefinedOperator::IntrinsicOperator* op);

  template <typename T>
  const Name* getNameFromClassWithDesignator(const T& t) {
    if (const auto* designator = std::get_if<Indirection<Designator>>(&t.u)) {
      if (const auto* dataRef = std::get_if<DataRef>(&designator->value().u)) {
        if (const auto* name = std::get_if<Name>(&dataRef->u)) {
          return name;
        }
      }
    }
    return nullptr;
  }

  const Symbol* getTypeSymbolFromSymbol(const Symbol* symbol);

  void handleTrackedVarAssignment(SourceName sourceName);

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

  void Post(const FunctionStmt& f);
  void Post(const EndFunctionStmt&);
  void Post(const SubroutineStmt& s);
  void Post(const EndSubroutineStmt&);

  void Post(const ProcedureDesignator& p);

  void Post(const AssignmentStmt& a);
  void Post(const AllocateStmt& a);
  void Post(const Call& c);

  // handle destructors (finalizers) TODO: test i definitely missed some edges cases
  void Post(const TypeDeclarationStmt& t);

  // following 5 methods are for collecting types and their procedures. see type struct and vector.

  // type def
  bool Pre(const DerivedTypeDef&);
  void Post(const DerivedTypeDef&);

  // type stmt like type [, extends(...)] :: body (not exhaustive and not extends)
  bool Pre(const DerivedTypeStmt& t);

  // type attrs like extends
  void Post(const TypeAttrSpec& a);

  // procedures in type defs
  void Post(const TypeBoundProcedureStmt& s);

  // collect defined operators in a type def (operator overloading)
  void Post(const TypeBoundGenericStmt& s);

  // the following 4 methods are for collecting defined operators in interface statements
  bool Pre(const InterfaceStmt&);
  bool Pre(const EndInterfaceStmt&);
  void Post(const DefinedOperator& op);
  void Post(const ProcedureStmt& p);

  // parse operators in expressions
  bool Pre(const Expr& e);
  void Post(const Expr& e);

 private:
  metacg::Callgraph* cg;
  std::vector<std::pair<std::string, std::string>> edges;  // (caller, callee)
  std::string currentFileName;

  bool inFunctionOrSubroutineSubProgram = false;
  bool inMainProgram = false;
  bool inDerivedTypeDef = false;
  bool inInterfaceStmt = false;
  bool inInterfaceStmtDefinedOperator = false;
  bool inInterfaceSpecification = false;

  std::vector<Symbol*> functionSymbols;
  std::vector<std::vector<const Name*>> functionDummyArgs;

  std::vector<type_t> types;

  std::vector<std::pair<const std::variant<DefinedOpName, DefinedOperator::IntrinsicOperator>*,
                        std::vector<Symbol*>>>
      interfaceOperators;  // operator name (symbol) => [procedure names (symbols)]

  std::vector<const Expr*> exprStmtWithOps;

  // mainly used for destructor handling
  std::vector<trackedVar_t> trackedVars;

  AL* al = AL::getInstance();
};
