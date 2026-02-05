#include "ParseTreeVisitor.h"

// util functions

template <typename T>
void ParseTreeVisitor::handleFuncSubStmt(const T& stmt) {
  if (auto* sym = std::get<Name>(stmt.t).symbol) {
    functionSymbols.emplace_back(sym);
    functionDummyArgs.emplace_back(std::vector<const Name*>());
    cg->insert(std::make_unique<metacg::CgNode>(mangleName(*sym), currentFileName, false, false));

    al->debug("Add node: {} ({})", mangleName(*sym), fmt::ptr(sym));
  }
}

template void ParseTreeVisitor::handleFuncSubStmt<FunctionStmt>(const FunctionStmt&);
template void ParseTreeVisitor::handleFuncSubStmt<SubroutineStmt>(const SubroutineStmt&);

void ParseTreeVisitor::handleEndFuncSubStmt() {
  handleTrackedVars();

  if (!functionSymbols.empty()) {
    functionSymbols.pop_back();
  }
  if (!functionDummyArgs.empty()) {
    functionDummyArgs.pop_back();
  }
}

void ParseTreeVisitor::handleTrackedVars() {
  if (mangleName(*functionSymbols.back()) != "_QQmain") {
    if (!trackedVars.empty())
      al->debug("Handle tracked vars for function");

    for (auto& trackedVar : trackedVars) {
      if (trackedVar.addFinalizers) {
        if (!trackedVar.hasBeenInitialized)
          continue;
        if (trackedVar.procedure != functionSymbols.back())
          continue;

        // add edge for deconstruction (finalizer)
        auto* typeSymbol = getTypeSymbolFromSymbol(trackedVar.var);
        if (!typeSymbol)
          continue;

        add_edges_for_finalizers(typeSymbol);
      } else {
      }
    }
  }

  // cleanup trackedVars
  trackedVars.erase(std::remove_if(trackedVars.begin(), trackedVars.end(),
                                   [&](const trackedVar_t& t) { return t.procedure == functionSymbols.back(); }),
                    trackedVars.end());
}

std::vector<type_t> ParseTreeVisitor::find_type_with_derived_types(const Symbol* typeSymbol) {
  std::vector<type_t> typeWithDerived;

  auto findTypeIt =
      std::find_if(types.begin(), types.end(), [&typeSymbol](const type_t& t) { return t.type == typeSymbol; });
  if (findTypeIt == types.end())
    return typeWithDerived;

  typeWithDerived.push_back(*findTypeIt);

  // base type
  if ((*findTypeIt).extendsFrom == nullptr) {
    for (auto t : types) {
      if (t.extendsFrom != typeSymbol)
        continue;

      typeWithDerived.push_back(t);
    }
    // not a base type, go back recursively to find all "base" types
  } else {
    auto* currentExtendsFrom = (*findTypeIt).extendsFrom;
    while (currentExtendsFrom) {
      auto currentType = std::find_if(types.begin(), types.end(),
                                      [&currentExtendsFrom](const type_t& t) { return t.type == currentExtendsFrom; });
      if (currentType == types.end()) {
        al->error("Error: Types array (extendsFrom) field entry missing.");
        return typeWithDerived;
      }

      typeWithDerived.push_back(*currentType);

      if ((*currentType).extendsFrom != nullptr) {
        currentExtendsFrom = (*currentType).extendsFrom;
      } else {
        currentExtendsFrom = nullptr;
      }
    }
  }

  return typeWithDerived;
}

void ParseTreeVisitor::add_edges_for_produces_and_derived_types(std::vector<type_t> typeWithDerived,
                                                                const Symbol* procedureSymbol) {
  for (type_t t : typeWithDerived) {
    auto procIt = std::find_if(t.procedures.begin(), t.procedures.end(), [&procedureSymbol](const auto& p) {
      return p.first->name() == procedureSymbol->name();
    });
    if (procIt == t.procedures.end())
      continue;

    edges.emplace_back(mangleName(*functionSymbols.back()), mangleName(*procIt->second));

    al->debug("Add edge: {} ({}) -> {} ({})", mangleName(*functionSymbols.back()), fmt::ptr(functionSymbols.back()),
              mangleName(*procIt->second), fmt::ptr(procIt->second));
  }
}

void ParseTreeVisitor::add_edges_for_finalizers(const Symbol* typeSymbol) {
  std::vector<type_t> typeSymbols = find_type_with_derived_types(typeSymbol);

  for (const auto& type : typeSymbols) {
    const Symbol* typeSymbol = type.type;

    const auto* details = std::get_if<DerivedTypeDetails>(&typeSymbol->details());
    if (!details)
      return;

    // add edges for finalizers
    for (const auto& final : details->finals()) {
      edges.emplace_back(mangleName(*functionSymbols.back()), mangleName(*final.second));

      al->debug("Add edge: {} ({}) -> {} ({})", mangleName(*functionSymbols.back()), fmt::ptr(functionSymbols.back()),
                mangleName(*final.second), fmt::ptr(&final.second.get()));
    }
  }
}

bool ParseTreeVisitor::isOperator(const Expr* e) {
  /* Operators: see 15.4.3.4.2, 10.1.6.1, 6.2.4 (https://j3-fortran.org/doc/year/23/23-007r1.pdf)
     Negate, NOT, Power, Multiply, Divide, Add, Subtract, Concat,
     LT, LE, EQ, NE, GE, GT, AND, OR, EQV, NEQV,
     DefinedUnary, DefinedBinary
   */

  return holds_any_of<decltype(e->u), Expr::UnaryPlus, Expr::Negate, Expr::NOT, Expr::Power, Expr::Multiply,
                      Expr::Divide, Expr::Add, Expr::Subtract, Expr::Concat, Expr::LT, Expr::LE, Expr::EQ, Expr::NE,
                      Expr::GE, Expr::GT, Expr::AND, Expr::OR, Expr::EQV, Expr::NEQV, Expr::DefinedUnary,
                      Expr::DefinedBinary>(e->u);
}

bool ParseTreeVisitor::compare_expr_IntrinsicOperator(const Expr* expr, const DefinedOperator::IntrinsicOperator* op) {
  if (!expr || !op)
    return false;

  using IO = DefinedOperator::IntrinsicOperator;

  switch (*op) {
    // case IO::UnaryPlus:
    //   return std::get_if<Expr::UnaryPlus>(&expr->u) != nullptr;
    // case IO::Negate:
    //   return std::get_if<Expr::Negate>(&expr->u) != nullptr;
    case IO::NOT:
      return std::get_if<Expr::NOT>(&expr->u) != nullptr;
    case IO::Power:
      return std::get_if<Expr::Power>(&expr->u) != nullptr;
    case IO::Multiply:
      return std::get_if<Expr::Multiply>(&expr->u) != nullptr;
    case IO::Divide:
      return std::get_if<Expr::Divide>(&expr->u) != nullptr;
    case IO::Add:
      return std::get_if<Expr::Add>(&expr->u) != nullptr;
    case IO::Subtract:
      return std::get_if<Expr::Subtract>(&expr->u) != nullptr;
    case IO::Concat:
      return std::get_if<Expr::Concat>(&expr->u) != nullptr;
    case IO::LT:
      return std::get_if<Expr::LT>(&expr->u) != nullptr;
    case IO::LE:
      return std::get_if<Expr::LE>(&expr->u) != nullptr;
    case IO::EQ:
      return std::get_if<Expr::EQ>(&expr->u) != nullptr;
    case IO::NE:
      return std::get_if<Expr::NE>(&expr->u) != nullptr;
    case IO::GE:
      return std::get_if<Expr::GE>(&expr->u) != nullptr;
    case IO::GT:
      return std::get_if<Expr::GT>(&expr->u) != nullptr;
    case IO::AND:
      return std::get_if<Expr::AND>(&expr->u) != nullptr;
    case IO::OR:
      return std::get_if<Expr::OR>(&expr->u) != nullptr;
    case IO::EQV:
      return std::get_if<Expr::EQV>(&expr->u) != nullptr;
    case IO::NEQV:
      return std::get_if<Expr::NEQV>(&expr->u) != nullptr;
    default:
      return false;
  }
}

const Symbol* ParseTreeVisitor::getTypeSymbolFromSymbol(const Symbol* symbol) {
  auto* type = symbol->GetType();
  if (!type)
    return nullptr;
  auto* derived = type->AsDerived();
  if (!derived)
    return nullptr;
  auto* typeSymbol = &derived->typeSymbol();
  if (!typeSymbol)
    return nullptr;
  return typeSymbol;
}

// search trackedVars for a canditate and set it as initialized.
// Prefers local variables when (shadowed)
void ParseTreeVisitor::handleTrackedVarAssignment(SourceName sourceName) {
  auto anyTrackedVarIt =
      std::find_if(trackedVars.begin(), trackedVars.end(), [&](const auto& t) { return t.var->name() == sourceName; });
  if (anyTrackedVarIt == trackedVars.end())
    return;

  // find local variable with the same name in the current function scope (shadowed)
  auto localVarIt = std::find_if(trackedVars.begin(), trackedVars.end(), [&](const auto& t) {
    return t.var->name() == sourceName && t.procedure == functionSymbols.back();
  });

  // prefer local var if found
  auto& trackedVar = (localVarIt != trackedVars.end()) ? *localVarIt : *anyTrackedVarIt;
  trackedVar.hasBeenInitialized = true;

  al->debug("Tracked var assigned: {} ({})", trackedVar.var->name(), fmt::ptr(trackedVar.var));
}

// Visitor implementations

bool ParseTreeVisitor::Pre(const MainProgram& p) {
  inMainProgram = true;

  if (const auto& maybeStmt = std::get<0>(p.t)) {
    if (!maybeStmt->statement.v.symbol)
      return true;

    functionSymbols.emplace_back(maybeStmt->statement.v.symbol);
    cg->insert(std::make_unique<metacg::CgNode>(mangleName(*functionSymbols.back()), currentFileName, false, false));

    al->debug("\nIn main program: {} ({})", mangleName(*functionSymbols.back()), fmt::ptr(functionSymbols.back()));
  }
  return true;
}

void ParseTreeVisitor::Post(const MainProgram&) {
  handleTrackedVars();

  al->debug("End main program: {} ({})", mangleName(*functionSymbols.back()), fmt::ptr(functionSymbols.back()));

  if (!functionSymbols.empty()) {
    functionSymbols.pop_back();
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

  auto* node = cg->getNode(mangleName(*functionSymbols.back()));
  if (!node) {
    return;
  }

  node->setHasBody(true);
}

void ParseTreeVisitor::Post(const EntryStmt& e) {
  auto* name = &std::get<Name>(e.t);
  if (!name->symbol)
    return;

  al->debug("Add Entry point: {} ({})", mangleName(*name->symbol), fmt::ptr(name->symbol));

  cg->insert(std::make_unique<metacg::CgNode>(mangleName(*name->symbol), currentFileName, false, true));
}

void ParseTreeVisitor::Post(const FunctionStmt& f) {
  al->debug("\nIn function: {} ({})", mangleName(*std::get<Name>(f.t).symbol), fmt::ptr(std::get<Name>(f.t).symbol));

  handleFuncSubStmt(f);

  // collect function arguments
  const auto& name_list = std::get<std::list<Name>>(f.t);
  for (auto name : name_list) {
    functionDummyArgs.back().push_back(&name);
  }
}

void ParseTreeVisitor::Post(const EndFunctionStmt&) {
  if (!functionSymbols.empty()) {
    al->debug("End function: {} ({})", mangleName(*functionSymbols.back()), fmt::ptr(functionSymbols.back()));
  }

  handleEndFuncSubStmt();
}

void ParseTreeVisitor::Post(const SubroutineStmt& s) {
  al->debug("\nIn subroutine: {} ({})", mangleName(*std::get<Name>(s.t).symbol), fmt::ptr(std::get<Name>(s.t).symbol));

  handleFuncSubStmt(s);

  // collect subroutine arguments (dummy args)
  const auto* dummyArg_list = &std::get<std::list<DummyArg>>(s.t);
  for (const auto& dummyArg : *dummyArg_list) {
    const auto* name = std::get_if<Name>(&dummyArg.u);
    functionDummyArgs.back().push_back(name);
  }
}

void ParseTreeVisitor::Post(const EndSubroutineStmt&) {
  if (!functionSymbols.empty()) {
    al->debug("End subroutine: {} ({})", mangleName(*functionSymbols.back()), fmt::ptr(functionSymbols.back()));
  }

  handleEndFuncSubStmt();
}

void ParseTreeVisitor::Post(const ProcedureDesignator& p) {
  if (functionSymbols.empty())
    return;

  // if just the name is called. (as subroutine with call and as function without call)
  if (auto* name = std::get_if<Name>(&p.u)) {
    if (!name->symbol)
      return;

    // ignore intrinsic functions
    if (name->symbol->attrs().test(Attr::INTRINSIC))
      return;

    edges.emplace_back(mangleName(*functionSymbols.back()), mangleName(*name->symbol));

    al->debug("Add edge: {} ({}) -> {} ({})", mangleName(*functionSymbols.back()), fmt::ptr(functionSymbols.back()),
              mangleName(*name->symbol), fmt::ptr(name->symbol));

    // if called from a object with %. (base % component)
  } else if (auto* procCompRef = std::get_if<ProcComponentRef>(&p.u)) {
    auto* symbolComp = procCompRef->v.thing.component.symbol;
    if (!symbolComp)
      return;

    edges.emplace_back(mangleName(*functionSymbols.back()), mangleName(*symbolComp));

    al->debug("Add edge: {} ({}) -> {} ({})", mangleName(*functionSymbols.back()), fmt::ptr(functionSymbols.back()),
              mangleName(*symbolComp), fmt::ptr(symbolComp));

    auto* baseName = std::get_if<Name>(&procCompRef->v.thing.base.u);
    if (!baseName || !baseName->symbol)
      return;
    auto* symbolBase = baseName->symbol;

    // handle derived types edges

    auto* typeSymbol = getTypeSymbolFromSymbol(symbolBase);
    if (!typeSymbol)
      return;

    add_edges_for_produces_and_derived_types(find_type_with_derived_types(typeSymbol), symbolComp);
  }
}

void ParseTreeVisitor::Post(const AssignmentStmt& a) {
  const auto* var = &std::get<Variable>(a.t);

  auto* name = getNameFromClassWithDesignator(*var);
  if (!name || !name->symbol)
    return;

  handleTrackedVarAssignment(name->symbol->name());
}

void ParseTreeVisitor::Post(const AllocateStmt& a) {
  const auto* allocs = &std::get<std::list<Allocation>>(a.t);

  for (const auto& alloc : *allocs) {
    const auto* allocObj = &std::get<AllocateObject>(alloc.t);
    const auto* name = std::get_if<Name>(&allocObj->u);
    if (!name || !name->symbol) {
      continue;
    }

    handleTrackedVarAssignment(name->symbol->name());
  }
}

void ParseTreeVisitor::Post(const Call& c) {
  // handle move_alloc intrinsic for allocatable vars
  const auto* designator = &std::get<ProcedureDesignator>(c.t);
  const auto* args = &std::get<std::list<ActualArgSpec>>(c.t);

  const auto* name = std::get_if<Name>(&designator->u);
  if (!name || !name->symbol)
    return;
  if (!name->symbol->attrs().test(Attr::INTRINSIC) && name->symbol->name() != "move_alloc")
    return;

  if (args->size() < 2)
    return;

  for (const auto& arg : *args) {
    const auto* actualArg = &std::get<ActualArg>(arg.t);
    const auto* expr = std::get_if<Indirection<Expr>>(&actualArg->u);
    if (!expr)
      return;
    auto* name = getNameFromClassWithDesignator(expr->value());
    if (!name || !name->symbol)
      return;

    handleTrackedVarAssignment(name->symbol->name());
  }
}

void ParseTreeVisitor::Post(const TypeDeclarationStmt& t) {
  if (functionSymbols.empty()) {
    // type declaration inside a module TODO:
    return;
  }

  for (const auto& entity : std::get<std::list<EntityDecl>>(t.t)) {
    const auto& name = std::get<ObjectName>(entity.t);
    if (!name.symbol)
      continue;

    // skip if name is an argument to a function or subroutine
    bool isFunctionArg = false;
    if (!functionDummyArgs.empty()) {
      auto it = std::find_if(functionDummyArgs.back().begin(), functionDummyArgs.back().end(),
                             [&name](const Name* dummyArg) { return dummyArg->symbol == name.symbol; });

      if (it != functionDummyArgs.back().end())
        isFunctionArg = true;
    }

    auto* typeSymbol = getTypeSymbolFromSymbol(name.symbol);
    if (!typeSymbol)
      continue;

    bool holds_allocatable = false;
    const IntentSpec* holds_intent = nullptr;
    for (const auto& attr : std::get<std::list<AttrSpec>>(t.t)) {
      if (std::holds_alternative<Allocatable>(attr.u))
        holds_allocatable = true;
      else if (std::holds_alternative<IntentSpec>(attr.u))
        holds_intent = &std::get<IntentSpec>(attr.u);
    }

    if (isFunctionArg) {
      if (!holds_allocatable) {
        if (!holds_intent) {
          // no intent attr, if not set does not call finalizer. Why? idk.
          trackedVars.push_back({name.symbol, functionSymbols.back(), false, true});
          al->debug("Add tracking for function argument: {} ({})", name.symbol->name(), fmt::ptr(name.symbol));
        } else {
          if (holds_intent->v == IntentSpec::Intent::Out) {
            // intent out, calls finalizer because (7.5.6.3 line 21 and onwards)
            add_edges_for_finalizers(typeSymbol);
          } else if (holds_intent->v == IntentSpec::Intent::InOut) {
            // intent inout, calls finalizer when set.
            trackedVars.push_back({name.symbol, functionSymbols.back(), false, true});
            al->debug("Add tracking for inout argument: {} ({})", name.symbol->name(), fmt::ptr(name.symbol));
          }
        }
      } else {
        // needs to be check at the end of prog TODO: allocatable attr as function argument
      }
    } else {
      if (holds_allocatable) {
        trackedVars.push_back({name.symbol, functionSymbols.back(), false, true});
        al->debug("Add tracking for allocatable variable: {} ({})", name.symbol->name(), fmt::ptr(name.symbol));
        // skip var with allocatable attr.
        // Add to trackedVars because it needs to be assigned at least once before calling a finalizers make sense.
      } else {
        add_edges_for_finalizers(typeSymbol);
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
  al->debug("End derived type: {} ({})", types.back().type->name(), fmt::ptr(types.back().type));
}

bool ParseTreeVisitor::Pre(const DerivedTypeStmt& t) {
  if (!inDerivedTypeDef)
    return true;

  auto& currentType = types.back();
  const auto& name = std::get<Name>(t.t);
  currentType.type = name.symbol;

  al->debug("\nIn derived type: {} ({})", currentType.type->name(), fmt::ptr(currentType.type));

  return true;
}

void ParseTreeVisitor::Post(const TypeAttrSpec& a) {
  if (!inDerivedTypeDef)
    return;

  auto& currentType = types.back();
  if (std::holds_alternative<TypeAttrSpec::Extends>(a.u)) {
    const auto& extends = std::get<TypeAttrSpec::Extends>(a.u);
    currentType.extendsFrom = extends.v.symbol;

    al->debug("Extends from: {} ({})", currentType.extendsFrom->name(), fmt::ptr(currentType.extendsFrom));
  }
}

void ParseTreeVisitor::Post(const TypeBoundProcedureStmt& s) {
  if (!inDerivedTypeDef)
    return;

  if (auto* withoutInterface = std::get_if<TypeBoundProcedureStmt::WithoutInterface>(&s.u)) {
    for (const auto& d : withoutInterface->declarations) {
      auto& name = std::get<Name>(d.t);
      if (!name.symbol)
        return;

      auto& optname = std::get<std::optional<Name>>(d.t);
      if (!optname || !optname->symbol) {
        return;
      }

      auto& currentType = types.back();
      currentType.procedures.emplace_back(name.symbol, optname->symbol);

      al->debug("Add procedure: {} ({}) -> {} ({})", name.symbol->name(), fmt::ptr(name.symbol),
                optname->symbol->name(), fmt::ptr(optname->symbol));
    }

    // only for abstract types, with deferred in binding attr list
  } else if (auto* withInterface = std::get_if<TypeBoundProcedureStmt::WithInterface>(&s.u)) {
    for (const auto& n : withInterface->bindingNames) {
      if (!n.symbol)
        return;

      auto& currentType = types.back();
      currentType.procedures.emplace_back(n.symbol, n.symbol);

      al->debug("Add procedure: {} ({}) -> {} ({})", n.symbol->name(), fmt::ptr(n.symbol), n.symbol->name(),
                fmt::ptr(n.symbol));
    }
  }
}

void ParseTreeVisitor::Post(const TypeBoundGenericStmt& s) {
  if (!inDerivedTypeDef)
    return;

  const auto& genericSpec = std::get<Indirection<GenericSpec>>(s.t);
  if (auto* definedOperator = std::get_if<DefinedOperator>(&genericSpec.value().u)) {
    if (auto* intrinsicOp = std::get_if<DefinedOperator::IntrinsicOperator>(&definedOperator->u)) {
      const auto& names = std::get<std::list<Name>>(s.t);

      auto& currentType = types.back();

      for (auto name : names) {
        if (!name.symbol)
          continue;

        currentType.operators.emplace_back(intrinsicOp, name.symbol);

        al->debug("Add operator: {} -> {} ({})", DefinedOperator::EnumToString(*intrinsicOp), name.symbol->name(),
                  fmt::ptr(name.symbol));
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

  interfaceOperators.emplace_back(&op.u, std::vector<Symbol*>());
}

void ParseTreeVisitor::Post(const ProcedureStmt& p) {
  if (!inInterfaceStmtDefinedOperator)
    return;

  auto name = std::get<std::list<Name>>(p.t);
  for (const auto& n : name) {
    if (!n.symbol)
      continue;

    if (interfaceOperators.empty()) {
      al->error("This should no happen. Likely there is a bug with parsing DefinedOperator's");
      continue;
    }

    interfaceOperators.back().second.push_back(n.symbol);
  }
}

bool ParseTreeVisitor::Pre(const Expr& e) {
  const auto* name = getNameFromClassWithDesignator(e);
  // true if not in a designator expr
  if (!name || !name->symbol || exprStmtWithOps.empty()) {
    if (isOperator(&e)) {
      exprStmtWithOps.emplace_back(&e);
    }

    return true;
  }

  for (auto e : exprStmtWithOps) {
    // search in interface operators TODO improve this just add everything in the interface instead of comparing
    // types
    auto it = std::find_if(interfaceOperators.begin(), interfaceOperators.end(), [&](const auto& p) {
      if (auto* intrinsicOp = std::get_if<DefinedOperator::IntrinsicOperator>(p.first)) {
        return compare_expr_IntrinsicOperator(e, intrinsicOp);
      }
      if (auto* definedOpName = std::get_if<DefinedOpName>(p.first)) {
        if (auto* definedUnary = std::get_if<Expr::DefinedUnary>(&e->u)) {
          auto* exprOpName = &std::get<DefinedOpName>(definedUnary->t);

          return definedOpName->v.symbol->name() == exprOpName->v.symbol->name();
        }
        if (auto* definedBinary = std::get_if<Expr::DefinedBinary>(&e->u)) {
        }
        return false;
      }
      return false;
    });
    if (it != interfaceOperators.end()) {
      // iterate over all procedures in interface (this vastly overestimates the calls). TODO: look into procdure
      // params to identify only the onces that could be called.
      for (auto* sym : it->second) {
        // skip self calls
        if (sym->name() == functionSymbols.back()->name())
          continue;

        edges.emplace_back(mangleName(*functionSymbols.back()), mangleName(*sym));

        al->debug("Add edge: {} ({}) -> {} ({})", mangleName(*functionSymbols.back()), fmt::ptr(functionSymbols.back()),
                  mangleName(*sym), fmt::ptr(sym));
      }
    }

    // search in derived types
    auto* typeSymbol = getTypeSymbolFromSymbol(name->symbol);
    if (!typeSymbol)
      continue;

    auto typeWithDerived = find_type_with_derived_types(typeSymbol);

    for (const auto& t : typeWithDerived) {
      auto opIt = std::find_if(t.operators.begin(), t.operators.end(),
                               [&](const auto& p) { return compare_expr_IntrinsicOperator(e, p.first); });
      if (opIt == t.operators.end())
        continue;

      auto funcSymbol = opIt->second;

      bool skipSelfCall = false;
      for (type_t t : typeWithDerived) {
        auto procIt = std::find_if(t.procedures.begin(), t.procedures.end(),
                                   [&funcSymbol](const auto& p) { return p.first->name() == funcSymbol->name(); });
        if (procIt == t.procedures.end())
          continue;

        if (procIt->second->name() == functionSymbols.back()->name()) {
          skipSelfCall = true;
          break;
        }
      }

      if (!skipSelfCall)
        add_edges_for_produces_and_derived_types(typeWithDerived, funcSymbol);
    }
  }

  return true;
}

void ParseTreeVisitor::Post(const Expr& e) {
  // find out if this is a constructor
  // auto* functionRef = std::get_if<Indirection<FunctionReference>>(&e.u);
  // if (functionRef) {
  //   auto* designator = &std::get<ProcedureDesignator>(functionRef->value().v.t);
  //   auto* name = std::get_if<Name>(&designator->u);
  //   if (!name || !name->symbol) {
  //     return;
  //   }
  // }

  if (!isOperator(&e)) {
    return;
  }

  if (!exprStmtWithOps.empty()) {
    exprStmtWithOps.pop_back();
  }
}
