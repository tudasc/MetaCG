#include "ParseTreeVisitor.h"

// private functions

static bool compareSymbols(const Symbol* a, const Symbol* b) {
  if (a == b)
    return true;
  if (!a || !b)
    return false;
  if (a->name() != b->name())
    return false;

  auto resolveHostAssoc = [](const Symbol* sym) -> const Symbol* {
    while (sym) {
      if (sym->has<HostAssocDetails>()) {
        sym = &sym->get<HostAssocDetails>().symbol();
      } else if (sym->has<UseDetails>()) {
        sym = &sym->get<UseDetails>().symbol();
      } else {
        break;
      }
    }
    return sym;
  };
  if (resolveHostAssoc(a) != resolveHostAssoc(b))
    return false;

  if (a->attrs() != b->attrs())
    return false;

  // this only compares only the type and not all details like variables, procedures, generics, etc. But should be
  // enough for now.
  if (a->GetType() != b->GetType())
    return false;

  return true;
}

// util functions

std::string ParseTreeVisitor::mangleSymbol(const Symbol* sym) {
  if (!sym) {
    al->error("Error: mangleSymbol called with nullptr");
    return "";
  }

  std::string mangledName = mangleName(*sym);

  // Legacy Fortran - C interoperability before BIND(C) existed.
  // I have to do this manually because normally it would run as
  // a pass (ExternalNameConversionPass).
  //
  // NOTE: underscoring can be disabled with `-fno-underscoring`
  auto result = fir::NameUniquer::deconstruct(mangledName);
  if (fir::NameUniquer::isExternalFacingUniquedName(result)) {
    if (result.first == fir::NameUniquer::NameKind::COMMON && result.second.name.empty())
      mangledName = Fortran::common::blankCommonObjectName;
    mangledName = Fortran::common::GetExternalAssemblyName(result.second.name, true);
  }

  return mangledName;
}

template <typename T>
void ParseTreeVisitor::handleFuncSubStmt(const T& stmt) {
  if (auto* sym = std::get<Name>(stmt.t).symbol) {
    functionSymbols.emplace_back(sym);
    functionDummyArgs.emplace_back(std::vector<const Name*>());
    cg->getOrInsertNode(mangleSymbol(sym), currentFileName, false, false);
    functions.push_back({sym, std::vector<function::dummyArg>()});

    al->debug("Add node: {} ({})", mangleSymbol(sym), fmt::ptr(sym));
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
  if (mangleSymbol(functionSymbols.back()) != "_QQmain") {
    if (!trackedVars.empty())
      al->debug("Handle tracked vars for function");

    for (auto& trackedVar : trackedVars) {
      if (!trackedVar.hasBeenInitialized)
        continue;
      if (trackedVar.procedure != functionSymbols.back())
        continue;

      // add edge for deconstruction (finalizer)
      if (trackedVar.addFinalizers) {
        auto* typeSymbol = getTypeSymbolFromSymbol(trackedVar.var);
        if (!typeSymbol)
          continue;
        addEdgesForFinalizers(typeSymbol);
      }

      // set init on dummy function args
      auto functionIt = std::find_if(functions.begin(), functions.end(),
                                     [&](const auto& f) { return f.symbol == functionSymbols.back(); });
      if (functionIt != functions.end()) {
        auto dummyArgIt = std::find_if(functionIt->dummyArgs.begin(), functionIt->dummyArgs.end(),
                                       [&](const auto& d) { return d.symbol == trackedVar.var; });
        if (dummyArgIt != functionIt->dummyArgs.end()) {
          dummyArgIt->hasBeenInitialized = true;
        }
      }
    }
  }

  // cleanup trackedVars
  removeTrackedVars(functionSymbols.back());
}

std::vector<const type*> ParseTreeVisitor::findTypeWithDerivedTypes(const Symbol* typeSymbol) {
  std::vector<const type*> typesWithDerived;
  std::unordered_set<const Symbol*> visited;

  auto findTypeIt =
      std::find_if(types.begin(), types.end(), [&typeSymbol](const type& t) { return t.type == typeSymbol; });

  if (findTypeIt == types.end()) {
    return typesWithDerived;
  }

  typesWithDerived.push_back(&(*findTypeIt));  // Add the initial type
  visited.insert(typeSymbol);

  // collect descendants
  std::function<void(const type*)> collectDescendants = [&](const type* parent) {
    for (const auto& t : types) {
      if (t.extendsFrom == parent->type && !visited.count(t.type)) {
        visited.insert(t.type);
        typesWithDerived.push_back(&t);
        collectDescendants(&t);  // recursive call to find further descendants
      }
    }
  };
  collectDescendants(&(*findTypeIt));

  // collect ancestors
  const Symbol* currentExtendsFrom = findTypeIt->extendsFrom;
  while (currentExtendsFrom) {
    // not sure if Fortran even allows this. But better be safe
    if (!visited.insert(currentExtendsFrom).second) {
      al->error("Error: Detected cyclic inheritance involving type \"" +
                (currentExtendsFrom ? currentExtendsFrom->name().ToString() : "null") + "\"");
      break;
    }

    auto currentTypeIt = std::find_if(types.begin(), types.end(),
                                      [&](const type& t) { return compareSymbols(t.type, currentExtendsFrom); });

    if (currentTypeIt == types.end()) {
      al->error("Error: Types array (extendsFrom) field entry for \"" +
                (currentExtendsFrom ? currentExtendsFrom->name().ToString() : "null") + "\" missing");
      break;
    }

    typesWithDerived.push_back(&(*currentTypeIt));
    currentExtendsFrom = currentTypeIt->extendsFrom;
  }

  return typesWithDerived;
}

void ParseTreeVisitor::addEdgesForProducesAndDerivedTypes(std::vector<const type*> typeWithDerived,
                                                          const Symbol* procedureSymbol) {
  for (const type* t : typeWithDerived) {
    auto procIt = std::find_if(t->procedures.begin(), t->procedures.end(), [&procedureSymbol](const auto& p) {
      return p.first->name() == procedureSymbol->name();
    });
    if (procIt == t->procedures.end())
      continue;

    edges.emplace_back(mangleSymbol(functionSymbols.back()), mangleSymbol(procIt->second));

    al->debug("Add edge: {} ({}) -> {} ({})", mangleSymbol(functionSymbols.back()), fmt::ptr(functionSymbols.back()),
              mangleSymbol(procIt->second), fmt::ptr(procIt->second));
  }
}

void ParseTreeVisitor::addEdgesForFinalizers(const Symbol* typeSymbol) {
  for (const auto& edge : getEdgesForFinalizers(typeSymbol)) {
    edges.emplace_back(mangleSymbol(edge.first), mangleSymbol(edge.second));

    al->debug("Add edge for finalizer: {} ({}) -> {} ({})", mangleSymbol(edge.first), fmt::ptr(edge.first),
              mangleSymbol(edge.second), fmt::ptr(edge.second));
  }
}

void ParseTreeVisitor::addEdgesForFinalizers(std::vector<edge>* edges, const Symbol* typeSymbol) {
  for (const auto& edge : getEdgesForFinalizers(typeSymbol)) {
    edges->emplace_back(mangleSymbol(edge.first), mangleSymbol(edge.second));
  }
}

std::vector<std::pair<Symbol*, const Symbol*>> ParseTreeVisitor::getEdgesForFinalizers(const Symbol* typeSymbol) {
  std::vector<std::pair<Symbol*, const Symbol*>> edges;
  std::vector<const type*> typeSymbols = findTypeWithDerivedTypes(typeSymbol);

  for (const type* type : typeSymbols) {
    const Symbol* typeSymbol = type->type;

    const auto* details = std::get_if<DerivedTypeDetails>(&typeSymbol->details());
    if (!details)
      continue;

    // add edges for finalizers
    for (const auto& final : details->finals()) {
      edges.emplace_back(functionSymbols.back(), &final.second.get());
    }
  }

  return edges;
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

bool ParseTreeVisitor::compareExprIntrinsicOperator(const Expr* expr, DefinedOperator::IntrinsicOperator op) {
  if (!expr)
    return false;

  using IO = DefinedOperator::IntrinsicOperator;

  switch (op) {
    case IO::NOT:
      return std::get_if<Expr::NOT>(&expr->u) != nullptr;
    case IO::Power:
      return std::get_if<Expr::Power>(&expr->u) != nullptr;
    case IO::Multiply:
      return std::get_if<Expr::Multiply>(&expr->u) != nullptr;
    case IO::Divide:
      return std::get_if<Expr::Divide>(&expr->u) != nullptr;
    case IO::Add:
      return std::get_if<Expr::Add>(&expr->u) != nullptr ||
             std::get_if<Expr::UnaryPlus>(&expr->u) != nullptr;  // UnaryPlus also uses +
    case IO::Subtract:
      return std::get_if<Expr::Subtract>(&expr->u) != nullptr ||
             std::get_if<Expr::Negate>(&expr->u) != nullptr;  // Negate also uses -
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

bool ParseTreeVisitor::isBinaryOperator(const Expr* e) {
  if (!e)
    return false;

  return holds_any_of<decltype(e->u), Expr::Power, Expr::Multiply, Expr::Divide, Expr::Add, Expr::Subtract,
                      Expr::Concat, Expr::LT, Expr::LE, Expr::EQ, Expr::NE, Expr::GE, Expr::GT, Expr::AND, Expr::OR,
                      Expr::EQV, Expr::NEQV, Expr::DefinedBinary>(e->u);
}

bool ParseTreeVisitor::isUnaryOperator(const Expr* e) {
  if (!e)
    return false;

  return holds_any_of<decltype(e->u), Expr::UnaryPlus, Expr::Negate, Expr::NOT, Expr::DefinedUnary>(e->u);
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

trackedVar* ParseTreeVisitor::getTrackedVarFromSourceName(SourceName sourceName) {
  auto anyTrackedVarIt =
      std::find_if(trackedVars.begin(), trackedVars.end(), [&](const auto& t) { return t.var->name() == sourceName; });
  if (anyTrackedVarIt == trackedVars.end())
    return nullptr;

  // find local variable with the same name in the current function scope (shadowed)
  auto localVarIt = std::find_if(trackedVars.begin(), trackedVars.end(), [&](const auto& t) {
    return t.var->name() == sourceName && t.procedure == functionSymbols.back();
  });

  // prefer local var if found
  return (localVarIt != trackedVars.end()) ? &(*localVarIt) : &(*anyTrackedVarIt);
}

void ParseTreeVisitor::handleTrackedVarAssignment(SourceName sourceName) {
  auto* trackedVar = getTrackedVarFromSourceName(sourceName);
  if (!trackedVar)
    return;

  trackedVar->hasBeenInitialized = true;

  al->debug("Tracked var assigned: {} ({})", trackedVar->var->name(), fmt::ptr(trackedVar->var));
}

void ParseTreeVisitor::addTrackedVar(trackedVar var) {
  auto it = std::find_if(trackedVars.begin(), trackedVars.end(), [&](const trackedVar& t) { return t.var == var.var; });
  if (it != trackedVars.end()) {
    // update info
    it->addFinalizers = var.addFinalizers;
    it->hasBeenInitialized = var.hasBeenInitialized;
    al->debug("Update tracked variable: {} ({})", var.var->name(), fmt::ptr(var.var));
    return;
  }

  trackedVars.push_back(var);
  al->debug("Add tracking for variable: {} ({})", var.var->name(), fmt::ptr(var.var));
}

void ParseTreeVisitor::removeTrackedVars(Symbol* procedureSymbol) {
  trackedVars.erase(std::remove_if(trackedVars.begin(), trackedVars.end(),
                                   [&](const trackedVar& t) { return t.procedure == procedureSymbol; }),
                    trackedVars.end());
}

template <typename Variant>
DefinedOperator::IntrinsicOperator ParseTreeVisitor::mapToIntrinsicOperator(const Variant& op) {
  return std::visit(visitors{[this](const RelationalOperator& op) {
                               using RO = RelationalOperator;
                               using IO = DefinedOperator::IntrinsicOperator;

                               switch (op) {
                                 case RO::LT:
                                   return IO::LT;
                                 case RO::LE:
                                   return IO::LE;
                                 case RO::EQ:
                                   return IO::EQ;
                                 case RO::NE:
                                   return IO::NE;
                                 case RO::GE:
                                   return IO::GE;
                                 case RO::GT:
                                   return IO::GT;
                                 default:
                                   al->error("Error: Unknown RelationalOperator in mapToIntrinsicOperator");
                                   return IO::LT;  // avoid warning
                               }
                             },
                             [this](const LogicalOperator& op) {
                               using LO = LogicalOperator;
                               using IO = DefinedOperator::IntrinsicOperator;

                               switch (op) {
                                 case LO::And:
                                   return IO::AND;
                                 case LO::Or:
                                   return IO::OR;
                                 case LO::Eqv:
                                   return IO::EQV;
                                 case LO::Neqv:
                                   return IO::NEQV;
                                 case LO::Not:
                                   return IO::NOT;
                                 default:
                                   al->error("Error: Unknown LogicalOperator in mapToIntrinsicOperator");
                                   return IO::AND;  // avoid warning
                               }
                             },
                             [this](const NumericOperator& op) {
                               using NO = NumericOperator;
                               using IO = DefinedOperator::IntrinsicOperator;

                               switch (op) {
                                 case NO::Power:
                                   return IO::Power;
                                 case NO::Multiply:
                                   return IO::Multiply;
                                 case NO::Divide:
                                   return IO::Divide;
                                 case NO::Add:
                                   return IO::Add;
                                 case NO::Subtract:
                                   return IO::Subtract;
                                 default:
                                   al->error("Error: Unknown NumericOperator in mapToIntrinsicOperator");
                                   return IO::Add;  // avoid warning
                               }
                             },
                             [this](const auto& op) {
                               al->error("Error: Unknown operator type in mapToIntrinsicOperator");
                               return DefinedOperator::IntrinsicOperator::Add;  // avoid warning
                             }},
                    op);
}

// Visitor implementations

bool ParseTreeVisitor::Pre(const MainProgram& p) {
  inMainProgram = true;

  if (const auto& maybeStmt = std::get<0>(p.t)) {
    if (!maybeStmt->statement.v.symbol)
      return true;

    functionSymbols.emplace_back(maybeStmt->statement.v.symbol);
    cg->getOrInsertNode(mangleSymbol(functionSymbols.back()), currentFileName, false, false);

    al->debug("\nIn main program: {} ({})", mangleSymbol(functionSymbols.back()), fmt::ptr(functionSymbols.back()));
  }
  return true;
}

void ParseTreeVisitor::Post(const MainProgram&) {
  handleTrackedVars();

  al->debug("End main program: {} ({})", mangleSymbol(functionSymbols.back()), fmt::ptr(functionSymbols.back()));

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

  auto* node = cg->getFirstNode(mangleSymbol(functionSymbols.back()));
  if (!node) {
    return;
  }

  node->setHasBody(true);
}

void ParseTreeVisitor::Post(const EntryStmt& e) {
  auto* name = &std::get<Name>(e.t);
  if (!name->symbol)
    return;

  al->debug("Add Entry point: {} ({})", mangleSymbol(name->symbol), fmt::ptr(name->symbol));

  cg->getOrInsertNode(mangleSymbol(name->symbol), currentFileName, false, true);
}

void ParseTreeVisitor::Post(const FunctionStmt& f) {
  al->debug("\nIn function: {} ({})", mangleSymbol(std::get<Name>(f.t).symbol), fmt::ptr(std::get<Name>(f.t).symbol));

  handleFuncSubStmt(f);

  auto functionsIt = std::find_if(functions.begin(), functions.end(),
                                  [&](const auto& func) { return func.symbol == functionSymbols.back(); });

  // collect function arguments
  const auto& name_list = std::get<std::list<Name>>(f.t);
  for (auto name : name_list) {
    functionDummyArgs.back().push_back(&name);
    if (functionsIt != functions.end()) {
      functionsIt->dummyArgs.push_back({name.symbol, false});
      addTrackedVar({name.symbol, functionSymbols.back(), false, false});
    }
  }
}

void ParseTreeVisitor::Post(const EndFunctionStmt&) {
  if (!functionSymbols.empty()) {
    al->debug("End function: {} ({})", mangleSymbol(functionSymbols.back()), fmt::ptr(functionSymbols.back()));
  }

  handleEndFuncSubStmt();
}

void ParseTreeVisitor::Post(const SubroutineStmt& s) {
  al->debug("\nIn subroutine: {} ({})", mangleSymbol(std::get<Name>(s.t).symbol), fmt::ptr(std::get<Name>(s.t).symbol));

  handleFuncSubStmt(s);

  auto functionsIt = std::find_if(functions.begin(), functions.end(),
                                  [&](const auto& func) { return func.symbol == functionSymbols.back(); });

  // collect subroutine arguments (dummy args)
  const auto* dummyArg_list = &std::get<std::list<DummyArg>>(s.t);
  for (const auto& dummyArg : *dummyArg_list) {
    const auto* name = std::get_if<Name>(&dummyArg.u);
    functionDummyArgs.back().push_back(name);
    if (functionsIt != functions.end()) {
      functionsIt->dummyArgs.push_back({name->symbol, false});
      addTrackedVar({name->symbol, functionSymbols.back(), false, false});
    }
  }
}

void ParseTreeVisitor::Post(const EndSubroutineStmt&) {
  if (!functionSymbols.empty()) {
    al->debug("End subroutine: {} ({})", mangleSymbol(functionSymbols.back()), fmt::ptr(functionSymbols.back()));
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

    edges.emplace_back(mangleSymbol(functionSymbols.back()), mangleSymbol(name->symbol));

    al->debug("Add edge: {} ({}) -> {} ({})", mangleSymbol(functionSymbols.back()), fmt::ptr(functionSymbols.back()),
              mangleSymbol(name->symbol), fmt::ptr(name->symbol));

    // if called from a object with %. (base % component)
  } else if (auto* procCompRef = std::get_if<ProcComponentRef>(&p.u)) {
    auto* symbolComp = procCompRef->v.thing.component.symbol;
    if (!symbolComp)
      return;

    edges.emplace_back(mangleSymbol(functionSymbols.back()), mangleSymbol(symbolComp));

    al->debug("Add edge: {} ({}) -> {} ({})", mangleSymbol(functionSymbols.back()), fmt::ptr(functionSymbols.back()),
              mangleSymbol(symbolComp), fmt::ptr(symbolComp));

    auto* baseName = std::get_if<Name>(&procCompRef->v.thing.base.u);
    if (!baseName || !baseName->symbol)
      return;
    auto* symbolBase = baseName->symbol;

    // handle derived types edges

    auto* typeSymbol = getTypeSymbolFromSymbol(symbolBase);
    if (!typeSymbol)
      return;

    addEdgesForProducesAndDerivedTypes(findTypeWithDerivedTypes(typeSymbol), symbolComp);
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
  const auto* designator = &std::get<ProcedureDesignator>(c.t);
  const auto* args = &std::get<std::list<ActualArgSpec>>(c.t);

  const auto* procName = std::get_if<Name>(&designator->u);
  if (!procName || !procName->symbol)
    return;

  std::size_t argPos = 0;
  for (const auto& arg : *args) {
    const auto* actualArg = &std::get<ActualArg>(arg.t);
    const auto* expr = std::get_if<Indirection<Expr>>(&actualArg->u);
    if (!expr)
      return;
    auto* name = getNameFromClassWithDesignator(expr->value());
    if (!name || !name->symbol)
      return;

    // handle move_alloc intrinsic for allocatable vars
    if (procName->symbol->attrs().test(Attr::INTRINSIC) && procName->symbol->name() == "move_alloc") {
      handleTrackedVarAssignment(name->symbol->name());
    } else {
      // handle finalizers for allocatable vars.
      // This collects info from variables that are parse as arguments to functions. Function are defined below the
      // execution part, so this need to be handled at the end of the parse tree traversal.
      auto* trackedVar = getTrackedVarFromSourceName(name->symbol->name());
      if (!trackedVar)
        continue;

      potentialFinalizer pf = {argPos, mangleSymbol(procName->symbol), std::vector<edge>()};
      addEdgesForFinalizers(&pf.finalizerEdges, getTypeSymbolFromSymbol(trackedVar->var));

      potentialFinalizers.push_back(pf);
      al->debug("Add potential finalizer for var: {} ({})", name->symbol->name(), fmt::ptr(name->symbol));
    }
  }
}

void ParseTreeVisitor::Post(const TypeDeclarationStmt& t) {
  if (functionSymbols.empty()) {
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
    bool holds_save = false;
    for (const auto& attr : std::get<std::list<AttrSpec>>(t.t)) {
      if (std::holds_alternative<Allocatable>(attr.u))
        holds_allocatable = true;
      else if (std::holds_alternative<IntentSpec>(attr.u))
        holds_intent = &std::get<IntentSpec>(attr.u);
      else if (std::holds_alternative<Save>(attr.u))
        holds_save = true;
    }

    if (holds_save)
      continue;  // vars with save attr are not destructed

    if (isFunctionArg) {
      if (!holds_allocatable) {
        if (!holds_intent) {
          // no intent attr, if not set does not call finalizer. Why? idk.
          al->debug("Add tracking for function argument: {} ({})", name.symbol->name(), fmt::ptr(name.symbol));
          addTrackedVar({name.symbol, functionSymbols.back(), false, true});
        } else {
          if (holds_intent->v == IntentSpec::Intent::Out) {
            // intent out, calls finalizer because (7.5.6.3 line 21 and onwards)
            addEdgesForFinalizers(typeSymbol);
          } else if (holds_intent->v == IntentSpec::Intent::InOut) {
            // intent inout, calls finalizer when set.
            al->debug("Add tracking for inout argument: {} ({})", name.symbol->name(), fmt::ptr(name.symbol));
            addTrackedVar({name.symbol, functionSymbols.back(), false, true});
          }
        }
      }
    } else {
      if (holds_allocatable) {
        al->debug("Add tracking for allocatable variable: {} ({})", name.symbol->name(), fmt::ptr(name.symbol));
        addTrackedVar({name.symbol, functionSymbols.back(), false, true});
        // skip var with allocatable attr.
        // Add to trackedVars because it needs to be assigned at least once before calling a finalizers make sense.
      } else {
        addEdgesForFinalizers(typeSymbol);
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

        currentType.operators.emplace_back(*intrinsicOp, name.symbol);

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

  if (std::holds_alternative<DefinedOperator::IntrinsicOperator>(op.u)) {
    auto intrinsicOp = std::get<DefinedOperator::IntrinsicOperator>(op.u);
    interfaceOperators.emplace_back(intrinsicOp, std::vector<Symbol*>());
  } else if (std::holds_alternative<DefinedOpName>(op.u)) {
    const auto& opName = std::get<DefinedOpName>(op.u);
    if (!opName.v.symbol)
      return;

    interfaceOperators.emplace_back(opName.v.symbol, std::vector<Symbol*>());
  }
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

  // not in a function. So no overloaded operator is called.
  if (functionSymbols.empty())
    return true;

  for (auto e : exprStmtWithOps) {
    // search in interfaceOperators first before search in derived types
    auto interfaceOp = std::find_if(interfaceOperators.begin(), interfaceOperators.end(), [&](const auto& op) {
      if (std::holds_alternative<DefinedOperator::IntrinsicOperator>(op.first)) {
        auto intrinsicOp = std::get<DefinedOperator::IntrinsicOperator>(op.first);
        return compareExprIntrinsicOperator(e, intrinsicOp);
      } else if (std::holds_alternative<Symbol*>(op.first)) {
        Symbol* definedOpNameSym = std::get<Symbol*>(op.first);
        if (const auto* definedUnary = std::get_if<Expr::DefinedUnary>(&e->u)) {
          const auto& exprOpName = std::get<0>(definedUnary->t);
          return definedOpNameSym->name() == exprOpName.v.symbol->name();
        } else if (auto* definedBinary = std::get_if<Expr::DefinedBinary>(&e->u)) {
          const auto& exprOpName = std::get<0>(definedBinary->t);
          return definedOpNameSym->name() == exprOpName.v.symbol->name();
        }
      }
      return false;
    });
    if (interfaceOp != interfaceOperators.end()) {
      bool isUnaryOp = isUnaryOperator(e);
      bool isBinaryOp = isBinaryOperator(e);

      for (auto* sym : interfaceOp->second) {
        // skip self calls
        if (mangleSymbol(sym) == mangleSymbol(functionSymbols.back()))
          continue;

        // if unary, add potential unary operators. Same for binary operators.
        auto functionIt =
            std::find_if(functions.begin(), functions.end(), [&](const auto& f) { return f.symbol == sym; });
        if (functionIt != functions.end()) {
          if ((!isUnaryOp || functionIt->dummyArgs.size() != 1) && (!isBinaryOp || functionIt->dummyArgs.size() != 2)) {
            continue;
          }
        }

        edges.emplace_back(mangleSymbol(functionSymbols.back()), mangleSymbol(sym));

        al->debug("Add edge: {} ({}) -> {} ({})", mangleSymbol(functionSymbols.back()),
                  fmt::ptr(functionSymbols.back()), mangleSymbol(sym), fmt::ptr(sym));
      }
    }

    // search in derived types
    auto* typeSymbol = getTypeSymbolFromSymbol(name->symbol);
    if (!typeSymbol)
      continue;

    auto typeWithDerived = findTypeWithDerivedTypes(typeSymbol);

    for (const type* t : typeWithDerived) {
      auto opIt = std::find_if(t->operators.begin(), t->operators.end(),
                               [&](const auto& p) { return compareExprIntrinsicOperator(e, p.first); });
      if (opIt == t->operators.end())
        continue;

      auto funcSymbol = opIt->second;

      bool skipSelfCall = false;
      for (const type* t : typeWithDerived) {
        auto procIt = std::find_if(t->procedures.begin(), t->procedures.end(),
                                   [&funcSymbol](const auto& p) { return p.first->name() == funcSymbol->name(); });
        if (procIt == t->procedures.end())
          continue;

        if (procIt->second->name() == functionSymbols.back()->name()) {
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
  auto* useSymbol = u.moduleName.symbol;

  al->debug("Use module: {} ({})", useSymbol->name(), fmt::ptr(useSymbol));

  if (const Scope* modScope = useSymbol->scope()) {
    for (const auto& pair : *modScope) {
      Symbol& symbol = *pair.second;

      // extract derived types from module and populate types var
      if (const auto* details = symbol.detailsIf<DerivedTypeDetails>()) {
        Symbol* extendsFrom = nullptr;
        std::vector<std::pair<Symbol*, Symbol*>> procedures;
        std::vector<std::pair<DefinedOperator::IntrinsicOperator, Symbol*>> operators;

        for (auto& pair : *symbol.scope()) {
          Symbol& component = *pair.second;

          // extends
          if (component.test(Symbol::Flag::ParentComp)) {
            extendsFrom = const_cast<Symbol*>(getTypeSymbolFromSymbol(&component));  // TODO: avoid const cast. ugly.
          }

          // type bound procedures
          if (component.has<ProcBindingDetails>()) {
            const auto& procDetails = component.get<ProcBindingDetails>();
            al->debug("Found procedure in module derived type: {} ({})", component.name(), fmt::ptr(&component));
            procedures.emplace_back(&component, &component);
          }

          // type generic operators
          if (GenericDetails* gen = component.detailsIf<GenericDetails>()) {
            if (!gen->kind().IsIntrinsicOperator())
              continue;

            DefinedOperator::IntrinsicOperator intrinsicOp = mapToIntrinsicOperator(gen->kind().u);

            if (gen->specificProcs().size() != 1)
              al->error("Type-bound generic more than one specific proc not handled. Should not happen.");

            Symbol* op_func_sym = nullptr;
            op_func_sym = const_cast<Symbol*>(&gen->specificProcs().front().get());

            al->debug("Found operator in module derived type: {} -> {} ({})",
                      DefinedOperator::EnumToString(intrinsicOp), op_func_sym->name(), fmt::ptr(op_func_sym));

            operators.push_back({intrinsicOp, op_func_sym});
          }
        }

        types.push_back({&symbol, extendsFrom, procedures, operators});
        al->debug("Found derived type in module: {} ({})", symbol.name(), fmt::ptr(&symbol));
      }

      // same but with interface operators
      if (const auto* gen = symbol.detailsIf<GenericDetails>()) {
        std::variant<Symbol*, DefinedOperator::IntrinsicOperator> interfaceOp;
        std::vector<Symbol*> procs;

        if (gen->kind().IsIntrinsicOperator()) {
          interfaceOp = mapToIntrinsicOperator(gen->kind().u);
          al->debug("Found interface operator in module: {}",
                    DefinedOperator::EnumToString(std::get<DefinedOperator::IntrinsicOperator>(interfaceOp)));
        } else if (gen->kind().IsDefinedOperator()) {
          interfaceOp = &symbol;
          al->debug("Found interface operator in module: {}", symbol.name());
        } else {
          continue;
        }

        for (const auto& p : gen->specificProcs()) {
          procs.push_back(const_cast<Symbol*>(&p.get()));
          al->debug("  with procedure: {} ({})", p.get().name(), fmt::ptr(&p.get()));
        }

        interfaceOperators.push_back({interfaceOp, procs});
      }

      // same but with functions
      if (const auto* details = symbol.detailsIf<SubprogramDetails>()) {
        if (!details->isFunction() && !details->isInterface())  // function and function dummy definition in interface
          continue;

        std::vector<function::dummyArg> dummyArgs;
        for (Symbol* arg : details->dummyArgs()) {
          dummyArgs.push_back({arg, false});
        }

        functions.push_back({&symbol, dummyArgs});
        al->debug("Found function in module: {} ({})", symbol.name(), fmt::ptr(&symbol));
      }
    }
  }

  al->debug("Finished Use module: {} ({})", useSymbol->name(), fmt::ptr(useSymbol));
}
