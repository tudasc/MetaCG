#include "ParseTreeVisitor.h"

// util functions

template <typename T>
void ParseTreeVisitor::handleFuncSubStmt(const T& stmt) {
  if (auto* sym = std::get<Name>(stmt.t).symbol) {
    functionSymbols.emplace_back(sym);
    functionDummyArgs.emplace_back(std::vector<const Name*>());
    cg->insert(
        std::make_unique<metacg::CgNode>(Fortran::lower::mangle::mangleName(*sym), currentFileName, false, false));

    llvm::outs() << "Add node: " << Fortran::lower::mangle::mangleName(*sym) << "\n";
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
  for (auto& trackedVar : trackedVars) {
    if (trackedVar.hasBeenInitialized) {
      if (trackedVar.procedure != functionSymbols.back()) {
        continue;
      }

      // add edge for deconstruction (finalizer)
      auto* typeSymbol = getTypeSymbolFromSymbol(trackedVar.var);
      if (!typeSymbol)
        continue;
      auto* details = std::get_if<DerivedTypeDetails>(&typeSymbol->details());
      if (!details)
        continue;

      for (const auto& final : details->finals()) {
        edges.emplace_back(Fortran::lower::mangle::mangleName(*functionSymbols.back()),
                           Fortran::lower::mangle::mangleName(*final.second));

        llvm::outs() << "Add edge: " << Fortran::lower::mangle::mangleName(*functionSymbols.back()) << " -> "
                     << Fortran::lower::mangle::mangleName(*final.second) << "\n";
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
        llvm::errs() << "Error: Types array (extendsFrom) field entry missing.";
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

    edges.emplace_back(Fortran::lower::mangle::mangleName(*functionSymbols.back()),
                       Fortran::lower::mangle::mangleName(*procIt->second));

    llvm::outs() << "Add edge: " << Fortran::lower::mangle::mangleName(*functionSymbols.back()) << " -> "
                 << Fortran::lower::mangle::mangleName(*procIt->second) << "\n";
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

// Visitor implementations

bool ParseTreeVisitor::Pre(const MainProgram& p) {
  inMainProgram = true;

  if (const auto& maybeStmt = std::get<0>(p.t)) {
    if (!maybeStmt->statement.v.symbol)
      return true;

    functionSymbols.emplace_back(maybeStmt->statement.v.symbol);
    cg->insert(std::make_unique<metacg::CgNode>(Fortran::lower::mangle::mangleName(*functionSymbols.back()),
                                                currentFileName, false, false));
  }
  return true;
}

void ParseTreeVisitor::Post(const MainProgram&) {
  inMainProgram = false;

  handleTrackedVars();

  if (!functionSymbols.empty()) {
    functionSymbols.pop_back();
  }
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

  auto* node = cg->getNode(Fortran::lower::mangle::mangleName(*functionSymbols.back()));
  if (!node) {
    return;
  }

  node->setHasBody(true);
}

void ParseTreeVisitor::Post(const FunctionStmt& f) {
  llvm::outs() << "In function: " << Fortran::lower::mangle::mangleName(*std::get<Name>(f.t).symbol) << "\n";

  handleFuncSubStmt(f);

  // collect function arguments
  const auto& name_list = std::get<std::list<Name>>(f.t);
  for (auto name : name_list) {
    functionDummyArgs.back().push_back(&name);
  }
}

void ParseTreeVisitor::Post(const EndFunctionStmt&) {
  if (!functionSymbols.empty()) {
    llvm::outs() << "End function: " << Fortran::lower::mangle::mangleName(*functionSymbols.back()) << "\n";
  }

  handleEndFuncSubStmt();
}

void ParseTreeVisitor::Post(const SubroutineStmt& s) {
  llvm::outs() << "In subroutine: " << Fortran::lower::mangle::mangleName(*std::get<Name>(s.t).symbol) << "\n";

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
    llvm::outs() << "End subroutine: " << Fortran::lower::mangle::mangleName(*functionSymbols.back()) << "\n";
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

    edges.emplace_back(Fortran::lower::mangle::mangleName(*functionSymbols.back()),
                       Fortran::lower::mangle::mangleName(*name->symbol));

    llvm::outs() << "Add edge: " << Fortran::lower::mangle::mangleName(*functionSymbols.back()) << " -> "
                 << Fortran::lower::mangle::mangleName(*name->symbol) << "\n";

    // if called from a object with %. (base % component)
  } else if (auto* procCompRef = std::get_if<ProcComponentRef>(&p.u)) {
    auto* symbolComp = procCompRef->v.thing.component.symbol;
    if (!symbolComp)
      return;

    edges.emplace_back(Fortran::lower::mangle::mangleName(*functionSymbols.back()),
                       Fortran::lower::mangle::mangleName(*symbolComp));

    llvm::outs() << "Add edge: " << Fortran::lower::mangle::mangleName(*functionSymbols.back()) << " -> "
                 << Fortran::lower::mangle::mangleName(*symbolComp) << "\n";

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

  auto trackedVarIt = std::find_if(trackedVars.begin(), trackedVars.end(),
                                   [&name](const auto& t) { return t.var->name() == name->symbol->name(); });
  if (trackedVarIt == trackedVars.end()) {
    return;
  }

  trackedVarIt->hasBeenInitialized = true;
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
    if (!functionDummyArgs.empty()) {
      auto it = std::find_if(functionDummyArgs.back().begin(), functionDummyArgs.back().end(),
                             [&name](const Name* dummyArg) { return dummyArg->symbol == name.symbol; });

      if (it != functionDummyArgs.back().end())
        continue;
    }

    auto* typeSymbol = getTypeSymbolFromSymbol(name.symbol);
    if (!typeSymbol)
      continue;

    const auto& attrSpec = std::get<std::list<AttrSpec>>(t.t);
    auto it = std::find_if(attrSpec.begin(), attrSpec.end(),
                           [](const AttrSpec& a) { return std::holds_alternative<Allocatable>(a.u); });
    if (it != attrSpec.end()) {
      trackedVars.push_back({name.symbol, functionSymbols.back(), false});
      llvm::outs() << "Track variable: " << name.symbol->name() << "\n";
      continue;  // skip var with allocatable attr
    }

    const auto* details = std::get_if<DerivedTypeDetails>(&typeSymbol->details());
    if (!details)
      continue;

    // add edges for finalizers
    for (const auto& final : details->finals()) {
      edges.emplace_back(Fortran::lower::mangle::mangleName(*functionSymbols.back()),
                         Fortran::lower::mangle::mangleName(*final.second));

      llvm::outs() << "Add edge: " << Fortran::lower::mangle::mangleName(*functionSymbols.back()) << " -> "
                   << Fortran::lower::mangle::mangleName(*final.second) << "\n";
    }
  }
}

bool ParseTreeVisitor::Pre(const DerivedTypeDef&) {
  inDerivedTypeDef = true;
  types.emplace_back();

  return true;
}

void ParseTreeVisitor::Post(const DerivedTypeDef&) { inDerivedTypeDef = false; }

void ParseTreeVisitor::Post(const DerivedTypeStmt& t) {
  if (!inDerivedTypeDef)
    return;

  auto& currentType = types.back();
  const auto& name = std::get<Name>(t.t);
  currentType.type = name.symbol;
}

void ParseTreeVisitor::Post(const TypeAttrSpec& a) {
  if (!inDerivedTypeDef)
    return;

  auto& currentType = types.back();
  if (std::holds_alternative<TypeAttrSpec::Extends>(a.u)) {
    const auto& extends = std::get<TypeAttrSpec::Extends>(a.u);
    currentType.extendsFrom = extends.v.symbol;
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
    }

    // only for abstract types, with deferred in binding attr list
  } else if (auto* withInterface = std::get_if<TypeBoundProcedureStmt::WithInterface>(&s.u)) {
    for (const auto& n : withInterface->bindingNames) {
      if (!n.symbol)
        return;

      auto& currentType = types.back();
      currentType.procedures.emplace_back(n.symbol, n.symbol);
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
      llvm::errs() << "This should no happen. Likely there is a bug with parsing DefinedOperator's\n";
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
          llvm::outs() << "definedbinary" << "\n";
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

        edges.emplace_back(Fortran::lower::mangle::mangleName(*functionSymbols.back()),
                           Fortran::lower::mangle::mangleName(*sym));

        llvm::outs() << "Add edge: " << Fortran::lower::mangle::mangleName(*functionSymbols.back()) << " -> "
                     << Fortran::lower::mangle::mangleName(*sym) << "\n";
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
  if (!isOperator(&e)) {
    return;
  }

  if (!exprStmtWithOps.empty()) {
    exprStmtWithOps.pop_back();
  }
}
