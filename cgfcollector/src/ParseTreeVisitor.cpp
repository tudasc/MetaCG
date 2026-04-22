/**
 * File: ParseTreeVisitor.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "ParseTreeVisitor.h"
#include "FortranUtil.h"

using namespace Fortran::parser;
using namespace Fortran::semantics;
using namespace Fortran::common;
using namespace metacg;

namespace metacg::cgfcollector {

template <typename Iterable, typename Extractor>
void ParseTreeVisitor::handleDummyArgs(const Iterable& items, Extractor extract) {
  const Symbol* currentFunctionSymbol = currentFunctions.back().symbol;

  auto it = std::find_if(functions.begin(), functions.end(),
                         [&](const Function& func) { return func.symbol == currentFunctionSymbol; });
  if (it == functions.end())
    return;

  for (const auto& item : items) {
    if (const Symbol* symbol = extract(item)) {
      currentFunctions.back().addDummyArg(symbol);
      it->addDummyArg(symbol);
      varTracking->addTrackedVar({symbol, currentFunctionSymbol});
    }
  }
}

template <typename T>
void ParseTreeVisitor::handleFuncSubStmt(const T& stmt) {
  if (const Symbol* sym = std::get<Name>(stmt.t).symbol) {
    currentFunctions.emplace_back(sym, std::vector<Function::DummyArg>());
    functions.emplace_back(sym, std::vector<Function::DummyArg>());
    cg->getOrInsertNode(mangleSymbol(sym, underscoring), currentFileName, false, false);

    MCGLogger::logDebug("Add node: {} ({}) ({})", mangleSymbol(sym, underscoring), getDetailsName(sym), fmt::ptr(sym));
  }
}

void ParseTreeVisitor::handleEndFuncSubStmt() {
  varTracking->handleTrackedVars(currentFunctions.back().symbol, edgeM);

  if (!currentFunctions.empty()) {
    currentFunctions.pop_back();
  }
}

void ParseTreeVisitor::addEdgesForProceduresAndDerivedTypes(std::vector<const Type*> typeWithDerived,
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
    auto calledIt = std::find_if(functions.begin(), functions.end(), [&](const Function& f) {
      return mangleSymbol(f.symbol, underscoring) == pf.procedureCalled;
    });
    if (calledIt == functions.end())
      continue;

    auto arg = calledIt->dummyArgs.begin() + pf.argPos;

    if (!arg->hasBeenInitialized)
      continue;

    edgeM->addEdgesForFinalizers(pf);
  }

  edgeM->uniquifyEdges();

  // add edges
  for (const Edge& edge : edges) {
    const CgNode& callerNode = cg->getOrInsertNode(edge.caller);
    const CgNode& calleeNode = cg->getOrInsertNode(edge.callee);

    cg->addEdge(callerNode, calleeNode);
  }

  // debug

  MCGLogger::logDebug("procedureOverwrites Map:");
  for (const auto& [key, values] : procedureOverwrites) {
    MCGLogger::logDebug("  Type: {} ({}) ({}), Name: {}", key.first ? key.first->name().ToString() : "nullptr",
                        getDetailsName(key.first), fmt::ptr(key.first), key.second);
    for (const auto& value : values) {
      MCGLogger::logDebug("    Overwrites: {} ({}) ({})", value->name(), getDetailsName(value), fmt::ptr(value));
    }
  }

  MCGLogger::logDebug("finalizers Map:");
  for (const auto& [key, values] : finalizers) {
    MCGLogger::logDebug("  Type: {} ({}) ({})", key->name(), getDetailsName(key), fmt::ptr(key));
    for (const auto& value : values) {
      MCGLogger::logDebug("    Finalizer: {} ({}) ({})", value->name(), getDetailsName(value), fmt::ptr(value));
    }
  }

  MCGLogger::logDebug("typeOperators Map:");
  for (const auto& [key, values] : typeOperators) {
    MCGLogger::logDebug("  Type: {} ({}) ({}), Operator: {}", key.first ? key.first->name().ToString() : "nullptr",
                        getDetailsName(key.first), fmt::ptr(key.first), key.second);
    for (const auto& value : values) {
      MCGLogger::logDebug("    Binding name: {}", value);
    }
  }

  MCGLogger::logDebug("Type vector:");
  for (const auto& type : types) {
    MCGLogger::logDebug("  Type: {} ({}) ({})", type.typeSymbol->name(), getDetailsName(type.typeSymbol),
                        fmt::ptr(type.typeSymbol));
    if (type.extendsFrom) {
      MCGLogger::logDebug("    Extends from: {} ({}) ({})", type.extendsFrom->name(), getDetailsName(type.extendsFrom),
                          fmt::ptr(type.extendsFrom));
    }
    for (const auto& proc : type.procedures) {
      MCGLogger::logDebug("    Procedure: {} ({}) ({}) -> {} ({}) ({})", proc.first->name(), getDetailsName(proc.first),
                          fmt::ptr(proc.first), proc.second->name(), getDetailsName(proc.second),
                          fmt::ptr(proc.second));
    }
    for (const auto& ops : type.operators) {
      MCGLogger::logDebug("    Operator: {} -> {} ({}) ({})", DefinedOperator::EnumToString(ops.first),
                          ops.second->name(), getDetailsName(ops.second), fmt::ptr(ops.second));
    }
  }
}

// Visitor implementations

bool ParseTreeVisitor::Pre(const MainProgram& p) {
  inMainProgram = true;

  if (const auto& optionalProgramStmt = std::get<0>(p.t)) {
    Symbol* mainProgramSymbol = optionalProgramStmt->statement.v.symbol;
    if (!mainProgramSymbol)
      return true;

    const Symbol* currentFunctionSymbol =
        currentFunctions.emplace_back(mainProgramSymbol, std::vector<Function::DummyArg>()).symbol;
    cg->getOrInsertNode(mangleSymbol(currentFunctionSymbol, underscoring), currentFileName, false, false);

    MCGLogger::logDebug("\nIn main program: {} ({}) ({})", mangleSymbol(currentFunctionSymbol, underscoring),
                        getDetailsName(currentFunctionSymbol), fmt::ptr(currentFunctionSymbol));
  }
  return true;
}

void ParseTreeVisitor::Post(const MainProgram&) {
  if (!currentFunctions.empty()) {
    const Symbol* currentFunctionSymbol = currentFunctions.back().symbol;
    MCGLogger::logDebug("End main program: {} ({}) ({})", mangleSymbol(currentFunctionSymbol, underscoring),
                        getDetailsName(currentFunctionSymbol), fmt::ptr(currentFunctionSymbol));
  }

  handleEndFuncSubStmt();

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

  CgNode* node = cg->getFirstNode(mangleSymbol(currentFunctions.back().symbol, underscoring));
  if (!node) {
    return;
  }

  node->setHasBody(true);
}

void ParseTreeVisitor::Post(const EntryStmt& e) {
  const Name* name = &std::get<Name>(e.t);
  if (!name->symbol)
    return;

  MCGLogger::logDebug("Add Entry point: {} ({}) ({})", mangleSymbol(name->symbol, underscoring),
                      getDetailsName(name->symbol), fmt::ptr(name->symbol));

  // handle entry statement as normal function.
  cg->getOrInsertNode(mangleSymbol(name->symbol, underscoring), currentFileName, false, true);
}

void ParseTreeVisitor::Post(const FunctionStmt& f) {
  MCGLogger::logDebug("\nIn function: {} ({}) ({})", mangleSymbol(std::get<Name>(f.t).symbol, underscoring),
                      getDetailsName(std::get<Name>(f.t).symbol), fmt::ptr(std::get<Name>(f.t).symbol));

  handleFuncSubStmt(f);

  handleDummyArgs(std::get<std::list<Name>>(f.t), [](const Name& name) { return name.symbol; });
}

void ParseTreeVisitor::Post(const EndFunctionStmt&) {
  if (!currentFunctions.empty()) {
    const Symbol* currentFunctionSymbol = currentFunctions.back().symbol;
    MCGLogger::logDebug("End function: {} ({}) ({})", mangleSymbol(currentFunctionSymbol, underscoring),
                        getDetailsName(currentFunctionSymbol), fmt::ptr(currentFunctionSymbol));
  }

  handleEndFuncSubStmt();
}

void ParseTreeVisitor::Post(const SubroutineStmt& s) {
  MCGLogger::logDebug("\nIn subroutine: {} ({}) ({})", mangleSymbol(std::get<Name>(s.t).symbol, underscoring),
                      getDetailsName(std::get<Name>(s.t).symbol), fmt::ptr(std::get<Name>(s.t).symbol));

  handleFuncSubStmt(s);

  handleDummyArgs(std::get<std::list<DummyArg>>(s.t), [](const DummyArg& name) {
    if (const Name* n = std::get_if<Name>(&name.u)) {
      return n->symbol;
    }
    return static_cast<Symbol*>(nullptr);
  });
}

void ParseTreeVisitor::Post(const EndSubroutineStmt&) {
  if (!currentFunctions.empty()) {
    const Symbol* currentFunctionSymbol = currentFunctions.back().symbol;
    MCGLogger::logDebug("End subroutine: {} ({}) ({})", mangleSymbol(currentFunctionSymbol, underscoring),
                        getDetailsName(currentFunctionSymbol), fmt::ptr(currentFunctionSymbol));
  }

  handleEndFuncSubStmt();
}

void ParseTreeVisitor::Post(const ProcedureDesignator& p) {
  if (currentFunctions.empty())
    return;

  const Symbol* currentFunctionSymbol = currentFunctions.back().symbol;

  // if just the name is called. (as subroutine with call or as function without call)
  if (const Name* name = std::get_if<Name>(&p.u)) {
    if (!name->symbol)
      return;

    // ignore intrinsic functions
    if (!includeInstrinsics && name->symbol->attrs().test(Attr::INTRINSIC)) {
      return;
    }

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

    auto baseTypeIt = std::find_if(types.begin(), types.end(), [&](const Type& t) {
      return compareSymbols(t.typeSymbol, symbolBase, CanonicalMode::ByType);
    });
    if (baseTypeIt == types.end())
      return;

    try {
      auto proc = procedureOverwrites.at({getAbsoluteBaseSymbol(types, &(*baseTypeIt)), symbolComp->name().ToString()});
      for (const Symbol* p : proc) {
        edgeM->addEdge(currentFunctionSymbol, p);
      }
    } catch (const std::out_of_range& e) {
      // no overwrites for this procedure, do nothing
    }
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

      MCGLogger::logDebug("Add potential finalizers for var: {} ({}) ({})", name->symbol->name(),
                          getDetailsName(name->symbol), fmt::ptr(name->symbol));
      PotentialFinalizer& pf = potentialFinalizers.emplace_back(argPos, mangleSymbol(procName->symbol, underscoring));
      auto baseTypeIt = std::find_if(types.begin(), types.end(), [&](const Type& t) {
        return compareSymbols(t.typeSymbol, name->symbol, CanonicalMode::ByType);
      });
      if (baseTypeIt == types.end())
        continue;

      try {
        auto final = finalizers.at(getAbsoluteBaseSymbol(types, &(*baseTypeIt)));
        for (const Symbol* f : final) {
          pf.addFinalizerEdge({mangleSymbol(currentFunctionSymbol, underscoring), mangleSymbol(f, underscoring)});
          MCGLogger::logDebug("  Potential finalizer edge: {} ({}) -> {} ({})",
                              mangleSymbol(currentFunctionSymbol, underscoring), getDetailsName(currentFunctionSymbol),
                              mangleSymbol(f, underscoring), getDetailsName(f));
        }
      } catch (const std::out_of_range& e) {
        // no finalizer for this type, do nothing
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

    // vars with save attr are not destructed
    if (holds_save) {
      continue;
    }

    const Symbol* currentFunctionSymbol = currentFunctions.back().symbol;

    if (isFunctionArg) {
      if (!holds_allocatable) {
        if (!holds_intent) {
          // no intent attr, if not set does not call finalizer.
          MCGLogger::logDebug("Add tracking for function argument: {} ({}) ({})", name.symbol->name(),
                              getDetailsName(name.symbol), fmt::ptr(name.symbol));
          varTracking->addTrackedVar({name.symbol, currentFunctionSymbol, false, true});
        } else {
          if (holds_intent->v == IntentSpec::Intent::Out) {
            // intent out, calls finalizer because (7.5.6.3 line 21 and onwards)
            edgeM->addEdgesForFinalizers(types, finalizers, currentFunctionSymbol, name.symbol);
          } else if (holds_intent->v == IntentSpec::Intent::InOut) {
            // intent inout, calls finalizer when set.
            MCGLogger::logDebug("Add tracking for inout argument: {} ({}) ({})", name.symbol->name(),
                                getDetailsName(name.symbol), fmt::ptr(name.symbol));
            varTracking->addTrackedVar({name.symbol, currentFunctionSymbol, false, true});
          }
        }
      }
    } else {
      if (holds_allocatable) {
        MCGLogger::logDebug("Add tracking for allocatable variable: {} ({}) ({})", name.symbol->name(),
                            getDetailsName(name.symbol), fmt::ptr(name.symbol));
        varTracking->addTrackedVar({name.symbol, currentFunctionSymbol, false, true});
        // skip var with allocatable attr.
        // Add to trackedVars because it needs to be assigned at least once before calling a finalizers make sense.
      } else {
        edgeM->addEdgesForFinalizers(types, finalizers, currentFunctionSymbol, name.symbol);
      }
    }
  }
}

bool ParseTreeVisitor::Pre(const DerivedTypeDef&) {
  inDerivedTypeDef = true;

  // initialize with empty `Type`
  types.emplace_back();

  return true;
}

void ParseTreeVisitor::Post(const DerivedTypeDef&) {
  inDerivedTypeDef = false;

  // finalizers (must be after extends discovery)
  Type& currentType = types.back();
  auto canon = canonicalizeSymbol(currentType.typeSymbol).symbol;
  if (auto* details = canon->detailsIf<DerivedTypeDetails>()) {
    for (const auto& final : details->finals()) {
      finalizers[getAbsoluteBaseSymbol(types, &currentType)].emplace_back(&final.second.get());
    }
  }

  MCGLogger::logDebug("End derived type: {} ({}) ({})", types.back().typeSymbol->name(),
                      getDetailsName(types.back().typeSymbol), fmt::ptr(types.back().typeSymbol));
}

bool ParseTreeVisitor::Pre(const DerivedTypeStmt& t) {
  if (!inDerivedTypeDef)
    return true;

  Type& currentType = types.back();
  const Name& name = std::get<Name>(t.t);
  currentType.typeSymbol = name.symbol;

  MCGLogger::logDebug("\nIn derived type: {} ({}) ({})", currentType.typeSymbol->name(),
                      getDetailsName(currentType.typeSymbol), fmt::ptr(currentType.typeSymbol));

  return true;
}

void ParseTreeVisitor::Post(const TypeAttrSpec& a) {
  if (!inDerivedTypeDef)
    return;

  Type& currentType = types.back();
  if (std::holds_alternative<TypeAttrSpec::Extends>(a.u)) {
    const TypeAttrSpec::Extends& extends = std::get<TypeAttrSpec::Extends>(a.u);
    currentType.extendsFrom = extends.v.symbol;

    MCGLogger::logDebug("Extends from: {} ({}) ({})", currentType.extendsFrom->name(),
                        getDetailsName(currentType.extendsFrom), fmt::ptr(currentType.extendsFrom));
  }
}

void ParseTreeVisitor::Post(const TypeBoundProcedureStmt& s) {
  if (!inDerivedTypeDef)
    return;

  // basically normal type-bound procedure statement
  if (const TypeBoundProcedureStmt::WithoutInterface* withoutInterface =
          std::get_if<TypeBoundProcedureStmt::WithoutInterface>(&s.u)) {
    for (const TypeBoundProcDecl& d : withoutInterface->declarations) {
      const Name& name = std::get<Name>(d.t);
      if (!name.symbol)
        return;

      Type& currentType = types.back();

      const std::optional<Name>& opt = std::get<std::optional<Name>>(d.t);
      const Symbol* value = opt ? opt->symbol : name.symbol;

      currentType.procedures.emplace_back(name.symbol, value);

      procedureOverwrites[{getAbsoluteBaseSymbol(types, &currentType), name.symbol->name().ToString()}].emplace_back(
          value);

      MCGLogger::logDebug("Add procedure: {} ({}) ({}) -> {} ({}) ({})", name.symbol->name(),
                          getDetailsName(name.symbol), fmt::ptr(name.symbol), value->name(), getDetailsName(value),
                          fmt::ptr(value));
    }

    // For abstract types. This is eqivalent to an abstract class in C++. In Fortran, you provide the signature of a
    // procedure for an abstract type in an extra interface block. And Flang wraps such definitions in a `WithInterface`
    // struct.
  } else if (const TypeBoundProcedureStmt::WithInterface* withInterface =
                 std::get_if<TypeBoundProcedureStmt::WithInterface>(&s.u)) {
    for (const Name& n : withInterface->bindingNames) {
      if (!n.symbol)
        return;

      Type& currentType = types.back();
      currentType.procedures.emplace_back(n.symbol, n.symbol);

      if (const ProcBindingDetails* bindingDetails = n.symbol->detailsIf<ProcBindingDetails>()) {
        procedureOverwrites[{getAbsoluteBaseSymbol(types, &currentType), n.symbol->name().ToString()}].emplace_back(
            &bindingDetails->symbol());
      }

      MCGLogger::logDebug("Add procedure: {} ({}) ({}) -> {} ({}) ({})", n.symbol->name(), fmt::ptr(n.symbol),
                          getDetailsName(n.symbol), n.symbol->name(), getDetailsName(n.symbol), fmt::ptr(n.symbol));
    }
  }
}

void ParseTreeVisitor::Post(const TypeBoundGenericStmt& s) {
  if (!inDerivedTypeDef)
    return;

  Type& currentType = types.back();

  // type-bound operators are defined as type-bound generic statements. Here we unpack them and add them
  // to the current type.
  const Indirection<GenericSpec>& genericSpec = std::get<Indirection<GenericSpec>>(s.t);
  if (const DefinedOperator* definedOperator = std::get_if<DefinedOperator>(&genericSpec.value().u)) {
    for (const auto n : std::get<std::list<Name>>(s.t)) {
      typeOperators[{getAbsoluteBaseSymbol(types, &currentType), getOperatorStringFromDefinedOperator(definedOperator)}]
          .emplace_back(n.symbol->name().ToString());
    }

    if (const DefinedOperator::IntrinsicOperator* intrinsicOp =
            std::get_if<DefinedOperator::IntrinsicOperator>(&definedOperator->u)) {
      const std::list<Name>& names = std::get<std::list<Name>>(s.t);

      for (const Name& name : names) {
        if (!name.symbol)
          continue;

        currentType.operators.emplace_back(*intrinsicOp, name.symbol);

        MCGLogger::logDebug("Add operator: {} -> {} ({}) ({})", DefinedOperator::EnumToString(*intrinsicOp),
                            name.symbol->name(), getDetailsName(name.symbol), fmt::ptr(name.symbol));
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

  // intrinsic operators with a predefined name, like +, *, etc.
  if (std::holds_alternative<DefinedOperator::IntrinsicOperator>(op.u)) {
    DefinedOperator::IntrinsicOperator intrinsicOp = std::get<DefinedOperator::IntrinsicOperator>(op.u);
    interfaceOperators.emplace_back(intrinsicOp, std::vector<const Symbol*>());
    // custom operators with a name
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
      MCGLogger::logError("This should not happen. Likely there is a bug with parsing DefinedOperator's");
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
        if (mangleSymbol(sym, underscoring) == mangleSymbol(currentFunctionSymbol, underscoring))
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

    // search in derived types. Handle polymorphic calls by adding edges for all derived types that have a procedure for
    // the operator.

    auto typeIt = std::find_if(types.begin(), types.end(), [&](const Type& t) {
      return compareSymbols(t.typeSymbol, name->symbol, CanonicalMode::ByType);
    });
    if (typeIt == types.end())
      continue;

    auto op = typeOperators.find({getAbsoluteBaseSymbol(types, &(*typeIt)), getOperatorStringFromExpr(e)});
    if (op == typeOperators.end())
      continue;

    for (const std::string& opBindingName : op->second) {
      // skip self calls
      if (opBindingName == currentFunctions.back().symbol->name().ToString()) {
        continue;
      }

      auto procOverwriteIt = procedureOverwrites.find({getAbsoluteBaseSymbol(types, &(*typeIt)), opBindingName});
      if (procOverwriteIt == procedureOverwrites.end())
        continue;

      // skip self call
      bool isSelfCall = false;
      for (const Symbol* overwriteSym : procOverwriteIt->second) {
        if (overwriteSym->name() == currentFunctions.back().symbol->name()) {
          isSelfCall = true;
          break;
        }
      }
      if (isSelfCall)
        continue;

      for (const Symbol* overwriteSym : procOverwriteIt->second) {
        edgeM->addEdge(currentFunctions.back().symbol, overwriteSym);
      }
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

  MCGLogger::logDebug("\nUse module: {} ({}) ({})", useSymbol->name(), getDetailsName(useSymbol), fmt::ptr(useSymbol));

  if (const Scope* modScope = useSymbol->scope()) {
    for (const auto& pair : *modScope) {
      const Symbol* symbol = &*pair.second;

      // extract derived types from module and populate types var
      if (const DerivedTypeDetails* details = symbol->detailsIf<DerivedTypeDetails>()) {
        MCGLogger::logDebug("Found derived type in module: {} ({}) ({})", symbol->name(), getDetailsName(symbol),
                            fmt::ptr(symbol));

        Type& currentType = types.emplace_back();
        currentType.typeSymbol = symbol;

        // extends
        if (const Symbol* extendsSymbol = details->GetParentComponent(*symbol->scope())) {
          if (const ObjectEntityDetails* objectDetails = extendsSymbol->detailsIf<ObjectEntityDetails>()) {
            if (const auto* typeSym = getTypeAsDerivedTypeSymbol(extendsSymbol)) {
              currentType.extendsFrom = typeSym;

              MCGLogger::logDebug("Found extends in module derived type: {} ({}) ({}) -> {} ({}) ({})", symbol->name(),
                                  getDetailsName(symbol), fmt::ptr(symbol), currentType.extendsFrom->name(),
                                  getDetailsName(currentType.extendsFrom), fmt::ptr(currentType.extendsFrom));
            }
          }
        }

        // finalizers (must be after extends discovery)
        auto canon = canonicalizeSymbol(symbol).symbol;
        if (auto* details = canon->detailsIf<DerivedTypeDetails>()) {
          for (const auto& final : details->finals()) {
            finalizers[getAbsoluteBaseSymbol(types, &currentType)].emplace_back(&final.second.get());
          }
        }

        for (auto pair : *symbol->scope()) {
          const Symbol* component = &*pair.second;

          // type-bound procedures
          if (const ProcBindingDetails* procDetails = component->detailsIf<ProcBindingDetails>()) {
            currentType.procedures.emplace_back(component, component);

            procedureOverwrites[{getAbsoluteBaseSymbol(types, &currentType), component->name().ToString()}]
                .emplace_back(&procDetails->symbol());

            MCGLogger::logDebug("Found procedure in module derived type: {} ({}) ({})", component->name(),
                                getDetailsName(component), fmt::ptr(&component));
          }

          // type-bound generic operators
          if (const GenericDetails* gen = component->detailsIf<GenericDetails>()) {
            for (const auto& p : gen->specificProcs()) {
              typeOperators[{getAbsoluteBaseSymbol(types, &currentType),
                             getOperatorStringFromGenericDetails(component, gen->kind())}]
                  .emplace_back(p.get().name().ToString());
            }

            if (!gen->kind().IsIntrinsicOperator())
              continue;

            DefinedOperator::IntrinsicOperator intrinsicOp = variantGetIntrinsicOperator(gen->kind());

            // that is not true
            if (gen->specificProcs().size() != 1)
              MCGLogger::logError("Type-bound generic more than one specific proc not handled. Should not happen.");

            const Symbol* op_func_sym = nullptr;
            op_func_sym = &gen->specificProcs().front().get();

            currentType.operators.emplace_back(intrinsicOp, op_func_sym);

            MCGLogger::logDebug("Found operator in module derived type: {} -> {} ({}) ({})",
                                DefinedOperator::EnumToString(intrinsicOp), op_func_sym->name(),
                                getDetailsName(op_func_sym), fmt::ptr(op_func_sym));
          }
        }
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
          MCGLogger::logDebug("Found interface operator in module: {} ({})", symbol->name(), getDetailsName(symbol));
        } else {
          continue;
        }

        for (const auto& p : gen->specificProcs()) {
          procs.push_back(&p.get());
          MCGLogger::logDebug("  with procedure: {} ({}) ({})", p.get().name(), getDetailsName(&p.get()),
                              fmt::ptr(&p.get()));
        }

        interfaceOperators.push_back({interfaceOp, procs});
      }

      // same but with functions
      if (const SubprogramDetails* details = symbol->detailsIf<SubprogramDetails>()) {
        // function and function dummy definition in interface
        if (!details->isFunction() && !details->isInterface())
          continue;

        std::vector<Function::DummyArg> dummyArgs;
        for (const Symbol* arg : details->dummyArgs()) {
          dummyArgs.emplace_back(arg, false);
        }

        functions.emplace_back(symbol, dummyArgs);
        MCGLogger::logDebug("Found function in module: {} ({}) ({})", symbol->name(), getDetailsName(symbol),
                            fmt::ptr(symbol));
      }
    }
  }

  MCGLogger::logDebug("Finished Use module: {} ({}) ({})", useSymbol->name(), getDetailsName(useSymbol),
                      fmt::ptr(useSymbol));
}

bool ParseTreeVisitor::Pre(const StmtFunctionStmt& s) {
  MCGLogger::logDebug("\nIn statement function: {} ({}) ({})", mangleSymbol(std::get<Name>(s.t).symbol, underscoring),
                      getDetailsName(std::get<Name>(s.t).symbol), fmt::ptr(std::get<Name>(s.t).symbol));

  handleFuncSubStmt(s);

  // hasBody flag
  CgNode* node = cg->getFirstNode(mangleSymbol(currentFunctions.back().symbol, underscoring));
  if (!node) {
    return true;
  }
  node->setHasBody(true);

  return true;
}

void ParseTreeVisitor::Post(const StmtFunctionStmt& s) {
  if (!currentFunctions.empty()) {
    const Symbol* currentFunctionSymbol = currentFunctions.back().symbol;
    MCGLogger::logDebug("End statement function: {} ({}) ({})", mangleSymbol(currentFunctionSymbol, underscoring),
                        getDetailsName(currentFunctionSymbol), fmt::ptr(currentFunctionSymbol));
  }

  handleEndFuncSubStmt();
}

}  // namespace metacg::cgfcollector
