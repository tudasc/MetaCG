/**
 * File: ParseTreeVisitor.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "ParseTreeVisitor.h"

using namespace Fortran::parser;
using namespace Fortran::semantics;
using namespace Fortran::common;
using namespace metacg;

namespace metacg::cgfcollector {

template void ParseTreeVisitor::handleFuncSubStmt<FunctionStmt>(const FunctionStmt&);
template void ParseTreeVisitor::handleFuncSubStmt<SubroutineStmt>(const SubroutineStmt&);

template <typename T>
void ParseTreeVisitor::handleFuncSubStmt(const T& stmt) {
  if (const Symbol* sym = std::get<Name>(stmt.t).symbol) {
    currentFunctions.emplace_back(sym, std::vector<Function::DummyArg>());
    functions.emplace_back(sym, std::vector<Function::DummyArg>());
    cg->getOrInsertNode(mangleSymbol(sym), currentFileName, false, false);

    MCGLogger::logDebug("Add node: {} ({})", mangleSymbol(sym), fmt::ptr(sym));
  }
}

void ParseTreeVisitor::handleEndFuncSubStmt() {
  varTracking->handleTrackedVars(currentFunctions.back().symbol, edgeM);

  if (!currentFunctions.empty()) {
    currentFunctions.pop_back();
  }
}

void ParseTreeVisitor::addEdgesForProducesAndDerivedTypes(std::vector<const Type*> typeWithDerived,
                                                          const Symbol* procedureSymbol) {
  for (const Type* t : typeWithDerived) {
    auto procIt = std::find_if(t->procedures.begin(), t->procedures.end(), [&procedureSymbol](const auto& p) {
      return p.first->name() == procedureSymbol->name();
    });
    if (procIt == t->procedures.end())
      continue;

    const Symbol* currentFunctionSymbol = currentFunctions.back().symbol;

    edgeM->addEdge(currentFunctionSymbol, procIt->second);
  }
}

void ParseTreeVisitor::postProcess() {
  // handle potential finalizers from function calls
  for (PotentialFinalizer pf : potentialFinalizers) {
    auto calledIt = std::find_if(functions.begin(), functions.end(),
                                 [&](const Function& f) { return mangleSymbol(f.symbol) == pf.procedureCalled; });
    if (calledIt == functions.end())
      continue;

    auto arg = calledIt->dummyArgs.begin() + pf.argPos;

    if (!arg->hasBeenInitialized)
      continue;

    edgeM->addEdgesForFinalizers(pf);
  }

  // sort unique edges
  std::sort(edges.begin(), edges.end());
  auto it = std::unique(edges.begin(), edges.end());
  edges.erase(it, edges.end());

  // add edges
  for (const Edge& edge : edges) {
    const CgNode& callerNode = cg->getOrInsertNode(edge.caller);
    const CgNode& calleeNode = cg->getOrInsertNode(edge.callee);

    cg->addEdge(callerNode, calleeNode);
  }
}

// Visitor implementations

bool ParseTreeVisitor::Pre(const MainProgram& p) {
  inMainProgram = true;

  if (const auto& maybeStmt = std::get<0>(p.t)) {
    if (!maybeStmt->statement.v.symbol)
      return true;

    const Symbol* currentFunctionSymbol =
        currentFunctions.emplace_back(maybeStmt->statement.v.symbol, std::vector<Function::DummyArg>()).symbol;
    cg->getOrInsertNode(mangleSymbol(currentFunctionSymbol), currentFileName, false, false);

    MCGLogger::logDebug("\nIn main program: {} ({})", mangleSymbol(currentFunctionSymbol),
                        fmt::ptr(currentFunctionSymbol));
  }
  return true;
}

void ParseTreeVisitor::Post(const MainProgram&) {
  varTracking->handleTrackedVars(currentFunctions.back().symbol, edgeM);

  const Symbol* currentFunctionSymbol = currentFunctions.back().symbol;

  MCGLogger::logDebug("End main program: {} ({})", mangleSymbol(currentFunctionSymbol),
                      fmt::ptr(currentFunctionSymbol));

  if (!currentFunctions.empty()) {
    currentFunctions.pop_back();
  }

  inMainProgram = false;
}

bool ParseTreeVisitor::Pre(const FunctionSubprogram&) {
  inFunctionOrSubroutineSubProgram = true;
  return true;
}

void ParseTreeVisitor::Post(const FunctionSubprogram&) { inFunctionOrSubroutineSubProgram = false; }

bool ParseTreeVisitor::Pre(const SubroutineSubprogram&) {
  inFunctionOrSubroutineSubProgram = true;
  return true;
}

void ParseTreeVisitor::Post(const SubroutineSubprogram&) { inFunctionOrSubroutineSubProgram = false; }

void ParseTreeVisitor::Post(const ExecutionPart& e) {
  if (!inFunctionOrSubroutineSubProgram && !inMainProgram)
    return;

  CgNode* node = cg->getFirstNode(mangleSymbol(currentFunctions.back().symbol));
  if (!node) {
    return;
  }

  node->setHasBody(true);
}

void ParseTreeVisitor::Post(const EntryStmt& e) {
  const Name* name = &std::get<Name>(e.t);
  if (!name->symbol)
    return;

  MCGLogger::logDebug("Add Entry point: {} ({})", mangleSymbol(name->symbol), fmt::ptr(name->symbol));

  // handle entry statement as normal function.
  cg->getOrInsertNode(mangleSymbol(name->symbol), currentFileName, false, true);
}

void ParseTreeVisitor::Post(const FunctionStmt& f) {
  MCGLogger::logDebug("\nIn function: {} ({})", mangleSymbol(std::get<Name>(f.t).symbol),
                      fmt::ptr(std::get<Name>(f.t).symbol));

  handleFuncSubStmt(f);

  const Symbol* currentFunctionSymbol = currentFunctions.back().symbol;

  auto functionsIt = std::find_if(functions.begin(), functions.end(),
                                  [&](const Function& func) { return func.symbol == currentFunctionSymbol; });

  // collect function arguments
  const std::list<Name>& name_list = std::get<std::list<Name>>(f.t);
  for (const Name& name : name_list) {
    currentFunctions.back().addDummyArg(name.symbol);
    if (functionsIt != functions.end()) {
      functionsIt->addDummyArg(name.symbol);
      varTracking->addTrackedVar({name.symbol, currentFunctionSymbol});
    }
  }
}

void ParseTreeVisitor::Post(const EndFunctionStmt&) {
  if (!currentFunctions.empty()) {
    const Symbol* currentFunctionSymbol = currentFunctions.back().symbol;

    MCGLogger::logDebug("End function: {} ({})", mangleSymbol(currentFunctionSymbol), fmt::ptr(currentFunctionSymbol));
  }

  handleEndFuncSubStmt();
}

void ParseTreeVisitor::Post(const SubroutineStmt& s) {
  MCGLogger::logDebug("\nIn subroutine: {} ({})", mangleSymbol(std::get<Name>(s.t).symbol),
                      fmt::ptr(std::get<Name>(s.t).symbol));

  handleFuncSubStmt(s);

  const Symbol* currentFunctionSymbol = currentFunctions.back().symbol;

  auto functionsIt = std::find_if(functions.begin(), functions.end(),
                                  [&](const Function& func) { return func.symbol == currentFunctionSymbol; });

  // collect subroutine arguments (dummy args)
  const std::list<DummyArg>* dummyArg_list = &std::get<std::list<DummyArg>>(s.t);
  for (const DummyArg& dummyArg : *dummyArg_list) {
    const Name* name = std::get_if<Name>(&dummyArg.u);
    currentFunctions.back().addDummyArg(name->symbol);
    if (functionsIt != functions.end()) {
      functionsIt->addDummyArg(name->symbol);
      varTracking->addTrackedVar({name->symbol, currentFunctionSymbol});
    }
  }
}

void ParseTreeVisitor::Post(const EndSubroutineStmt&) {
  if (!currentFunctions.empty()) {
    const Symbol* currentFunctionSymbol = currentFunctions.back().symbol;

    MCGLogger::logDebug("End subroutine: {} ({})", mangleSymbol(currentFunctionSymbol),
                        fmt::ptr(currentFunctionSymbol));
  }

  handleEndFuncSubStmt();
}

void ParseTreeVisitor::Post(const ProcedureDesignator& p) {
  if (currentFunctions.empty())
    return;

  const Symbol* currentFunctionSymbol = currentFunctions.back().symbol;

  // if just the name is called. (as subroutine with call and as function without call)
  if (const Name* name = std::get_if<Name>(&p.u)) {
    if (!name->symbol)
      return;

    // ignore intrinsic functions
    if (name->symbol->attrs().test(Attr::INTRINSIC))
      return;

    edgeM->addEdge(currentFunctionSymbol, name->symbol);

    // if called from a object with %. (base % component)
  } else if (const ProcComponentRef* procCompRef = std::get_if<ProcComponentRef>(&p.u)) {
    const Symbol* symbolComp = procCompRef->v.thing.component.symbol;
    if (!symbolComp)
      return;

    edgeM->addEdge(currentFunctionSymbol, symbolComp);

    const Name* baseName = std::get_if<Name>(&procCompRef->v.thing.base.u);
    if (!baseName || !baseName->symbol)
      return;
    const Symbol* symbolBase = baseName->symbol;

    // handle derived types edges

    addEdgesForProducesAndDerivedTypes(findTypeWithDerivedTypes(types, symbolBase), symbolComp);
  }
}

void ParseTreeVisitor::Post(const AssignmentStmt& a) {
  const Variable* var = &std::get<Variable>(a.t);

  const Name* name = getNameFromClassWithDesignator(*var);
  if (!name || !name->symbol)
    return;

  varTracking->handleTrackedVarAssignment(currentFunctions.back().symbol, name->symbol->name());
}

void ParseTreeVisitor::Post(const AllocateStmt& a) {
  const std::list<Allocation>* allocs = &std::get<std::list<Allocation>>(a.t);

  for (const Allocation& alloc : *allocs) {
    const AllocateObject* allocObj = &std::get<AllocateObject>(alloc.t);
    const Name* name = std::get_if<Name>(&allocObj->u);
    if (!name || !name->symbol) {
      continue;
    }

    varTracking->handleTrackedVarAssignment(currentFunctions.back().symbol, name->symbol->name());
  }
}

void ParseTreeVisitor::Post(const Call& c) {
  const ProcedureDesignator* designator = &std::get<ProcedureDesignator>(c.t);
  const std::list<ActualArgSpec>* args = &std::get<std::list<ActualArgSpec>>(c.t);
  const Symbol* currentFunctionSymbol = currentFunctions.back().symbol;

  const Name* procName = std::get_if<Name>(&designator->u);
  if (!procName || !procName->symbol)
    return;

  std::size_t argPos = 0;
  for (const ActualArgSpec& arg : *args) {
    const ActualArg* actualArg = &std::get<ActualArg>(arg.t);
    const Indirection<Expr>* expr = std::get_if<Indirection<Expr>>(&actualArg->u);
    if (!expr)
      return;
    const Name* name = getNameFromClassWithDesignator(expr->value());
    if (!name || !name->symbol)
      return;

    // handle move_alloc intrinsic for allocatable vars
    if (procName->symbol->attrs().test(Attr::INTRINSIC) && procName->symbol->name() == "move_alloc") {
      varTracking->handleTrackedVarAssignment(currentFunctionSymbol, name->symbol->name());
    } else {
      // handle finalizers for allocatable vars.
      // This collects info from variables that are parse as arguments to functions. Function are defined below the
      // execution part, so this need to be handled at the end of the parse tree traversal.
      TrackedVar* trackedVar = varTracking->getTrackedVarFromSourceName(currentFunctionSymbol, name->symbol->name());
      if (!trackedVar)
        continue;

      MCGLogger::logDebug("Add potential finalizers for var: {} ({})", name->symbol->name(), fmt::ptr(name->symbol));
      PotentialFinalizer& pf = potentialFinalizers.emplace_back(argPos, mangleSymbol(procName->symbol));
      for (const EdgeSymbol& edge : edgeM->getEdgesForFinalizers(types, currentFunctionSymbol, trackedVar->var)) {
        pf.addFinalizerEdge({mangleSymbol(edge.caller), mangleSymbol(edge.callee)});
        MCGLogger::logDebug("  Potential finalizer edge: {} -> {}", mangleSymbol(edge.caller),
                            mangleSymbol(edge.callee));
      }
    }
  }
}

void ParseTreeVisitor::Post(const TypeDeclarationStmt& t) {
  if (currentFunctions.empty()) {
    return;
  }

  for (const EntityDecl& entity : std::get<std::list<EntityDecl>>(t.t)) {
    const Name& name = std::get<ObjectName>(entity.t);
    if (!name.symbol)
      continue;

    // skip if name is an argument to a function or subroutine
    bool isFunctionArg = false;
    std::vector<Function::DummyArg>& currentDummyArgs = currentFunctions.back().dummyArgs;
    if (!currentDummyArgs.empty()) {
      auto it = std::find_if(currentDummyArgs.begin(), currentDummyArgs.end(),
                             [&name](const Function::DummyArg& dummyArg) { return dummyArg.symbol == name.symbol; });

      if (it != currentDummyArgs.end())
        isFunctionArg = true;
    }

    bool holds_allocatable = false;
    const IntentSpec* holds_intent = nullptr;
    bool holds_save = false;
    for (const AttrSpec& attr : std::get<std::list<AttrSpec>>(t.t)) {
      if (std::holds_alternative<Allocatable>(attr.u))
        holds_allocatable = true;
      else if (std::holds_alternative<IntentSpec>(attr.u))
        holds_intent = &std::get<IntentSpec>(attr.u);
      else if (std::holds_alternative<Save>(attr.u))
        holds_save = true;
    }

    if (holds_save)
      continue;  // vars with save attr are not destructed

    const Symbol* currentFunctionSymbol = currentFunctions.back().symbol;

    if (isFunctionArg) {
      if (!holds_allocatable) {
        if (!holds_intent) {
          // no intent attr, if not set does not call finalizer. Why? idk.
          MCGLogger::logDebug("Add tracking for function argument: {} ({})", name.symbol->name(),
                              fmt::ptr(name.symbol));
          varTracking->addTrackedVar({name.symbol, currentFunctionSymbol, false, true});
        } else {
          if (holds_intent->v == IntentSpec::Intent::Out) {
            // intent out, calls finalizer because (7.5.6.3 line 21 and onwards)
            edgeM->addEdgesForFinalizers(types, currentFunctionSymbol, name.symbol);
          } else if (holds_intent->v == IntentSpec::Intent::InOut) {
            // intent inout, calls finalizer when set.
            MCGLogger::logDebug("Add tracking for inout argument: {} ({})", name.symbol->name(), fmt::ptr(name.symbol));
            varTracking->addTrackedVar({name.symbol, currentFunctionSymbol, false, true});
          }
        }
      }
    } else {
      if (holds_allocatable) {
        MCGLogger::logDebug("Add tracking for allocatable variable: {} ({})", name.symbol->name(),
                            fmt::ptr(name.symbol));
        varTracking->addTrackedVar({name.symbol, currentFunctionSymbol, false, true});
        // skip var with allocatable attr.
        // Add to trackedVars because it needs to be assigned at least once before calling a finalizers make sense.
      } else {
        edgeM->addEdgesForFinalizers(types, currentFunctionSymbol, name.symbol);
      }
    }
  }
}

bool ParseTreeVisitor::Pre(const DerivedTypeDef&) {
  inDerivedTypeDef = true;
  types.emplace_back();

  return true;
}

void ParseTreeVisitor::Post(const DerivedTypeDef&) {
  inDerivedTypeDef = false;
  MCGLogger::logDebug("End derived type: {} ({})", types.back().typeSymbol->name(), fmt::ptr(types.back().typeSymbol));
}

bool ParseTreeVisitor::Pre(const DerivedTypeStmt& t) {
  if (!inDerivedTypeDef)
    return true;

  Type& currentType = types.back();
  const Name& name = std::get<Name>(t.t);
  currentType.typeSymbol = name.symbol;

  MCGLogger::logDebug("\nIn derived type: {} ({})", currentType.typeSymbol->name(), fmt::ptr(currentType.typeSymbol));

  return true;
}

void ParseTreeVisitor::Post(const TypeAttrSpec& a) {
  if (!inDerivedTypeDef)
    return;

  Type& currentType = types.back();
  if (std::holds_alternative<TypeAttrSpec::Extends>(a.u)) {
    const TypeAttrSpec::Extends& extends = std::get<TypeAttrSpec::Extends>(a.u);
    currentType.extendsFrom = extends.v.symbol;

    MCGLogger::logDebug("Extends from: {} ({})", currentType.extendsFrom->name(), fmt::ptr(currentType.extendsFrom));
  }
}

void ParseTreeVisitor::Post(const TypeBoundProcedureStmt& s) {
  if (!inDerivedTypeDef)
    return;

  if (const TypeBoundProcedureStmt::WithoutInterface* withoutInterface =
          std::get_if<TypeBoundProcedureStmt::WithoutInterface>(&s.u)) {
    for (const TypeBoundProcDecl& d : withoutInterface->declarations) {
      const Name& name = std::get<Name>(d.t);
      if (!name.symbol)
        return;

      const std::optional<Name>& optname = std::get<std::optional<Name>>(d.t);
      if (!optname || !optname->symbol) {
        return;
      }

      Type& currentType = types.back();
      currentType.procedures.emplace_back(name.symbol, optname->symbol);

      MCGLogger::logDebug("Add procedure: {} ({}) -> {} ({})", name.symbol->name(), fmt::ptr(name.symbol),
                          optname->symbol->name(), fmt::ptr(optname->symbol));
    }

    // only for abstract types, with deferred in binding attr list
  } else if (const TypeBoundProcedureStmt::WithInterface* withInterface =
                 std::get_if<TypeBoundProcedureStmt::WithInterface>(&s.u)) {
    for (const Name& n : withInterface->bindingNames) {
      if (!n.symbol)
        return;

      Type& currentType = types.back();
      currentType.procedures.emplace_back(n.symbol, n.symbol);

      MCGLogger::logDebug("Add procedure: {} ({}) -> {} ({})", n.symbol->name(), fmt::ptr(n.symbol), n.symbol->name(),
                          fmt::ptr(n.symbol));
    }
  }
}

void ParseTreeVisitor::Post(const TypeBoundGenericStmt& s) {
  if (!inDerivedTypeDef)
    return;

  const Indirection<GenericSpec>& genericSpec = std::get<Indirection<GenericSpec>>(s.t);
  if (const DefinedOperator* definedOperator = std::get_if<DefinedOperator>(&genericSpec.value().u)) {
    if (const DefinedOperator::IntrinsicOperator* intrinsicOp =
            std::get_if<DefinedOperator::IntrinsicOperator>(&definedOperator->u)) {
      const std::list<Name>& names = std::get<std::list<Name>>(s.t);

      Type& currentType = types.back();

      for (const Name& name : names) {
        if (!name.symbol)
          continue;

        currentType.operators.emplace_back(*intrinsicOp, name.symbol);

        MCGLogger::logDebug("Add operator: {} -> {} ({})", DefinedOperator::EnumToString(*intrinsicOp),
                            name.symbol->name(), fmt::ptr(name.symbol));
      }
    }
  }
}

bool ParseTreeVisitor::Pre(const InterfaceStmt&) {
  inInterfaceStmt = true;
  return true;
}

bool ParseTreeVisitor::Pre(const EndInterfaceStmt&) {
  inInterfaceStmt = false;
  inInterfaceStmtDefinedOperator = false;
  return true;
}

void ParseTreeVisitor::Post(const DefinedOperator& op) {
  if (!inInterfaceStmt)
    return;

  inInterfaceStmtDefinedOperator = true;

  if (std::holds_alternative<DefinedOperator::IntrinsicOperator>(op.u)) {
    DefinedOperator::IntrinsicOperator intrinsicOp = std::get<DefinedOperator::IntrinsicOperator>(op.u);
    interfaceOperators.emplace_back(intrinsicOp, std::vector<const Symbol*>());
  } else if (std::holds_alternative<DefinedOpName>(op.u)) {
    const DefinedOpName& opName = std::get<DefinedOpName>(op.u);
    if (!opName.v.symbol)
      return;

    interfaceOperators.emplace_back(opName.v.symbol, std::vector<const Symbol*>());
  }
}

void ParseTreeVisitor::Post(const ProcedureStmt& p) {
  if (!inInterfaceStmtDefinedOperator)
    return;

  const std::list<Name>& name = std::get<std::list<Name>>(p.t);
  for (const Name& n : name) {
    if (!n.symbol)
      continue;

    if (interfaceOperators.empty()) {
      MCGLogger::logError("This should no happen. Likely there is a bug with parsing DefinedOperator's");
      continue;
    }

    interfaceOperators.back().second.push_back(n.symbol);
  }
}

bool ParseTreeVisitor::Pre(const Expr& e) {
  const Name* name = getNameFromClassWithDesignator(e);
  // true if not in a designator expr
  if (!name || !name->symbol || exprStmtWithOps.empty()) {
    if (isOperator(&e)) {
      exprStmtWithOps.emplace_back(&e);
    }

    return true;
  }

  // not in a function. So no overloaded operator is called.
  if (currentFunctions.empty())
    return true;

  for (const Expr* e : exprStmtWithOps) {
    // search in interfaceOperators first before search in derived types
    auto interfaceOp = std::find_if(interfaceOperators.begin(), interfaceOperators.end(), [&](const auto& op) {
      if (std::holds_alternative<DefinedOperator::IntrinsicOperator>(op.first)) {
        DefinedOperator::IntrinsicOperator intrinsicOp = std::get<DefinedOperator::IntrinsicOperator>(op.first);
        return compareExprIntrinsicOperator(e, intrinsicOp);
      } else if (std::holds_alternative<const Symbol*>(op.first)) {
        const Symbol* definedOpNameSym = std::get<const Symbol*>(op.first);
        if (const Expr::DefinedUnary* definedUnary = std::get_if<Expr::DefinedUnary>(&e->u)) {
          const auto& exprOpName = std::get<0>(definedUnary->t);
          return definedOpNameSym->name() == exprOpName.v.symbol->name();
        } else if (const Expr::DefinedBinary* definedBinary = std::get_if<Expr::DefinedBinary>(&e->u)) {
          const auto& exprOpName = std::get<0>(definedBinary->t);
          return definedOpNameSym->name() == exprOpName.v.symbol->name();
        }
      }
      return false;
    });
    if (interfaceOp != interfaceOperators.end()) {
      bool isUnaryOp = isUnaryOperator(e);
      bool isBinaryOp = isBinaryOperator(e);

      for (const Symbol* sym : interfaceOp->second) {
        const Symbol* currentFunctionSymbol = currentFunctions.back().symbol;

        // skip self calls
        if (mangleSymbol(sym) == mangleSymbol(currentFunctionSymbol))
          continue;

        // if unary, add potential unary operators. Same for binary operators.
        auto functionIt =
            std::find_if(functions.begin(), functions.end(), [&](const Function& f) { return f.symbol == sym; });
        if (functionIt != functions.end()) {
          if ((!isUnaryOp || functionIt->dummyArgs.size() != 1) && (!isBinaryOp || functionIt->dummyArgs.size() != 2)) {
            continue;
          }
        }

        edgeM->addEdge(currentFunctionSymbol, sym);
      }
    }

    // search in derived types

    std::vector<const Type*> typeWithDerived = findTypeWithDerivedTypes(types, name->symbol);

    for (const Type* t : typeWithDerived) {
      auto opIt = std::find_if(t->operators.begin(), t->operators.end(),
                               [&](const auto& p) { return compareExprIntrinsicOperator(e, p.first); });
      if (opIt == t->operators.end())
        continue;

      const Symbol* funcSymbol = opIt->second;

      bool skipSelfCall = false;
      for (const Type* t : typeWithDerived) {
        auto procIt = std::find_if(t->procedures.begin(), t->procedures.end(),
                                   [&funcSymbol](const auto& p) { return p.first->name() == funcSymbol->name(); });
        if (procIt == t->procedures.end())
          continue;

        if (procIt->second->name() == currentFunctions.back().symbol->name()) {
          skipSelfCall = true;
          break;
        }
      }

      if (!skipSelfCall)
        addEdgesForProducesAndDerivedTypes(typeWithDerived, funcSymbol);
    }
  }

  return true;
}

void ParseTreeVisitor::Post(const Expr& e) {
  if (!isOperator(&e)) {
    return;
  }

  if (!exprStmtWithOps.empty()) {
    exprStmtWithOps.pop_back();
  }
}

void ParseTreeVisitor::Post(const UseStmt& u) {
  const Symbol* useSymbol = u.moduleName.symbol;

  MCGLogger::logDebug("Use module: {} ({})", useSymbol->name(), fmt::ptr(useSymbol));

  if (const Scope* modScope = useSymbol->scope()) {
    for (const auto& pair : *modScope) {
      const Symbol* symbol = &*pair.second;

      // extract derived types from module and populate types var
      if (const DerivedTypeDetails* details = symbol->detailsIf<DerivedTypeDetails>()) {
        const Symbol* extendsFrom = nullptr;
        std::vector<std::pair<const Symbol*, const Symbol*>> procedures;
        std::vector<std::pair<DefinedOperator::IntrinsicOperator, const Symbol*>> operators;

        for (auto pair : *symbol->scope()) {
          const Symbol* component = &*pair.second;

          // extends
          if (component->test(Symbol::Flag::ParentComp)) {
            extendsFrom = getTypeSymbolFromSymbol(component);
          }

          // type bound procedures
          if (component->has<ProcBindingDetails>()) {
            const ProcBindingDetails& procDetails = component->get<ProcBindingDetails>();
            MCGLogger::logDebug("Found procedure in module derived type: {} ({})", component->name(),
                                fmt::ptr(&component));
            procedures.emplace_back(component, component);
          }

          // type generic operators
          if (const GenericDetails* gen = component->detailsIf<GenericDetails>()) {
            if (!gen->kind().IsIntrinsicOperator())
              continue;

            DefinedOperator::IntrinsicOperator intrinsicOp = variantGetIntrinsicOperator(gen->kind());

            if (gen->specificProcs().size() != 1)
              MCGLogger::logError("Type-bound generic more than one specific proc not handled. Should not happen.");

            const Symbol* op_func_sym = nullptr;
            op_func_sym = &gen->specificProcs().front().get();

            MCGLogger::logDebug("Found operator in module derived type: {} -> {} ({})",
                                DefinedOperator::EnumToString(intrinsicOp), op_func_sym->name(), fmt::ptr(op_func_sym));

            operators.push_back({intrinsicOp, op_func_sym});
          }
        }

        types.push_back({symbol, extendsFrom, procedures, operators});
        MCGLogger::logDebug("Found derived type in module: {} ({})", symbol->name(), fmt::ptr(symbol));
      }

      // same but with interface operators
      if (const GenericDetails* gen = symbol->detailsIf<GenericDetails>()) {
        std::variant<const Symbol*, DefinedOperator::IntrinsicOperator> interfaceOp;
        std::vector<const Symbol*> procs;

        if (gen->kind().IsIntrinsicOperator()) {
          interfaceOp = variantGetIntrinsicOperator(gen->kind());
          MCGLogger::logDebug("Found interface operator in module: {}",
                              DefinedOperator::EnumToString(std::get<DefinedOperator::IntrinsicOperator>(interfaceOp)));
        } else if (gen->kind().IsDefinedOperator()) {
          interfaceOp = symbol;
          MCGLogger::logDebug("Found interface operator in module: {}", symbol->name());
        } else {
          continue;
        }

        for (const auto& p : gen->specificProcs()) {
          procs.push_back(&p.get());
          MCGLogger::logDebug("  with procedure: {} ({})", p.get().name(), fmt::ptr(&p.get()));
        }

        interfaceOperators.push_back({interfaceOp, procs});
      }

      // same but with functions
      if (const SubprogramDetails* details = symbol->detailsIf<SubprogramDetails>()) {
        if (!details->isFunction() && !details->isInterface())  // function and function dummy definition in interface
          continue;

        std::vector<Function::DummyArg> dummyArgs;
        for (const Symbol* arg : details->dummyArgs()) {
          dummyArgs.emplace_back(arg, false);
        }

        functions.emplace_back(symbol, dummyArgs);
        MCGLogger::logDebug("Found function in module: {} ({})", symbol->name(), fmt::ptr(symbol));
      }
    }
  }

  MCGLogger::logDebug("Finished Use module: {} ({})", useSymbol->name(), fmt::ptr(useSymbol));
}

}  // namespace metacg::cgfcollector
