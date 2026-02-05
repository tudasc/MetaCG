#include "headers.h"

typedef struct type {
  Fortran::semantics::Symbol* type;
  Fortran::semantics::Symbol* extendsFrom;
  std::vector<std::pair<Fortran::semantics::Symbol*, Fortran::semantics::Symbol*>>
      procedures;  // name(symbol) => optname(symbol)
  std::vector<std::pair<const Fortran::parser::DefinedOperator::IntrinsicOperator*, Fortran::semantics::Symbol*>>
      operators;  // operator => name(symbol)
} type_t;

class ParseTreeVisitor {
 public:
  ParseTreeVisitor(metacg::Callgraph* cg, std::string currentFileName) : cg(cg), currentFileName(currentFileName) {};

  template <typename T>
  void handleFuncSubStmt(const T& stmt) {
    if (auto* sym = std::get<Fortran::parser::Name>(stmt.t).symbol) {
      functionSymbols.emplace_back(sym);
      functionDummyArgs.emplace_back(std::vector<const Fortran::parser::Name*>());
      cg->insert(
          std::make_unique<metacg::CgNode>(Fortran::lower::mangle::mangleName(*sym), currentFileName, false, false));

      llvm::outs() << "Add node: " << Fortran::lower::mangle::mangleName(*sym) << "\n";
    }
  }
  void handleEndFuncSubStmt() {
    if (!functionSymbols.empty()) {
      functionSymbols.pop_back();
    }
    if (!functionDummyArgs.empty()) {
      functionDummyArgs.pop_back();
    }
  }

  std::vector<std::pair<std::string, std::string>> getEdges() const { return edges; }

  // searches the types vector for given typeSymbol and returns pointers to vectors of type with derived types.
  std::vector<type_t> find_type_with_derived_types(const Fortran::semantics::Symbol* typeSymbol) {
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
        auto currentType = std::find_if(types.begin(), types.end(), [&currentExtendsFrom](const type_t& t) {
          return t.type == currentExtendsFrom;
        });
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

  // this function searches with typeSymbol for a type in types vector and adds edges for procedures that matches
  // procedureSymbol. And also adds edges from types that extends from typeSymbol.
  void add_edges_for_produces_and_derived_types(std::vector<type_t> typeWithDerived,
                                                const Fortran::semantics::Symbol* procedureSymbol) {
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

  template <typename A>
  bool Pre(const A&) {
    return true;
  }
  template <typename A>
  void Post(const A&) {}

  bool Pre(const Fortran::parser::MainProgram& p) {
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

  void Post(const Fortran::parser::MainProgram&) {
    inMainProgram = false;

    if (!functionSymbols.empty()) {
      functionSymbols.pop_back();
    }
  }

  bool Pre(const Fortran::parser::FunctionSubprogram&) {
    inFunctionOrSubroutineSubProgram = true;
    return true;
  }
  void Post(const Fortran::parser::FunctionSubprogram&) { inFunctionOrSubroutineSubProgram = false; }
  bool Pre(const Fortran::parser::SubroutineSubprogram&) {
    inFunctionOrSubroutineSubProgram = true;
    return true;
  }
  void Post(const Fortran::parser::SubroutineSubprogram&) { inFunctionOrSubroutineSubProgram = false; }

  void Post(const Fortran::parser::ExecutionPart& e) {
    if (!inFunctionOrSubroutineSubProgram && !inMainProgram)
      return;

    auto* node = cg->getNode(Fortran::lower::mangle::mangleName(*functionSymbols.back()));
    if (!node) {
      return;
    }

    node->setHasBody(true);
  }

  void Post(const Fortran::parser::FunctionStmt& f) {
    llvm::outs() << "In function: " << Fortran::lower::mangle::mangleName(*std::get<Fortran::parser::Name>(f.t).symbol)
                 << "\n";

    handleFuncSubStmt(f);

    // collect function arguments
    const auto& name_list = std::get<std::list<Fortran::parser::Name>>(f.t);
    for (auto name : name_list) {
      functionDummyArgs.back().push_back(&name);
    }
  }
  void Post(const Fortran::parser::EndFunctionStmt&) {
    if (!functionSymbols.empty()) {
      llvm::outs() << "End function: " << Fortran::lower::mangle::mangleName(*functionSymbols.back()) << "\n";
    }

    handleEndFuncSubStmt();
  }
  void Post(const Fortran::parser::SubroutineStmt& s) {
    llvm::outs() << "In subroutine: "
                 << Fortran::lower::mangle::mangleName(*std::get<Fortran::parser::Name>(s.t).symbol) << "\n";

    handleFuncSubStmt(s);

    // collect subroutine arguments (dummy args)
    const auto* dummyArg_list = &std::get<std::list<Fortran::parser::DummyArg>>(s.t);
    for (const auto& dummyArg : *dummyArg_list) {
      const auto* name = std::get_if<Fortran::parser::Name>(&dummyArg.u);
      functionDummyArgs.back().push_back(name);
    }
  }
  void Post(const Fortran::parser::EndSubroutineStmt&) {
    if (!functionSymbols.empty()) {
      llvm::outs() << "End subroutine: " << Fortran::lower::mangle::mangleName(*functionSymbols.back()) << "\n";
    }

    handleEndFuncSubStmt();
  }

  void Post(const Fortran::parser::ProcedureDesignator& p) {
    if (functionSymbols.empty())
      return;

    // if just the name is called. (as subroutine with call and as function without call)
    if (auto* name = std::get_if<Fortran::parser::Name>(&p.u)) {
      if (!name->symbol)
        return;

      // ignore intrinsic functions
      if (name->symbol->attrs().test(Fortran::semantics::Attr::INTRINSIC))
        return;

      edges.emplace_back(Fortran::lower::mangle::mangleName(*functionSymbols.back()),
                         Fortran::lower::mangle::mangleName(*name->symbol));

      llvm::outs() << "Add edge: " << Fortran::lower::mangle::mangleName(*functionSymbols.back()) << " -> "
                   << Fortran::lower::mangle::mangleName(*name->symbol) << "\n";

      // if called from a object with %. (base % component)
    } else if (auto* procCompRef = std::get_if<Fortran::parser::ProcComponentRef>(&p.u)) {
      auto* symbolComp = procCompRef->v.thing.component.symbol;
      if (!symbolComp)
        return;

      edges.emplace_back(Fortran::lower::mangle::mangleName(*functionSymbols.back()),
                         Fortran::lower::mangle::mangleName(*symbolComp));

      llvm::outs() << "Add edge: " << Fortran::lower::mangle::mangleName(*functionSymbols.back()) << " -> "
                   << Fortran::lower::mangle::mangleName(*symbolComp) << "\n";

      auto* baseName = std::get_if<Fortran::parser::Name>(&procCompRef->v.thing.base.u);
      if (!baseName || !baseName->symbol)
        return;
      auto* symbolBase = baseName->symbol;

      // handle derived types edges

      auto* type = symbolBase->GetType();
      if (!type)
        return;
      auto* derived = type->AsDerived();
      if (!derived)
        return;
      auto* typeSymbol = &derived->typeSymbol();
      if (!typeSymbol)
        return;

      add_edges_for_produces_and_derived_types(find_type_with_derived_types(typeSymbol), symbolComp);
    }
  }

  // handle destructors (finalizers) TODO: test i definitely missed some edges cases
  void Post(const Fortran::parser::TypeDeclarationStmt& t) {
    // TODO: allocatable case
    // const auto& attrSpec = std::get<std::list<Fortran::parser::AttrSpec>>(t.t);
    // for (const auto& attr : attrSpec) {
    //   if (std::holds_alternative<Fortran::parser::Allocatable>(attr.u)) {
    //     return;  // skip allocatable because no finalizer called
    //   }
    // }

    for (const auto& entity : std::get<std::list<Fortran::parser::EntityDecl>>(t.t)) {
      const auto& name = std::get<Fortran::parser::ObjectName>(entity.t);
      if (!name.symbol)
        continue;

      // skip if name is an argument to a function or subroutine
      if (!functionDummyArgs.empty()) {
        auto it =
            std::find_if(functionDummyArgs.back().begin(), functionDummyArgs.back().end(),
                         [&name](const Fortran::parser::Name* dummyArg) { return dummyArg->symbol == name.symbol; });

        if (it != functionDummyArgs.back().end())
          continue;
      }

      auto* type = name.symbol->GetType();
      if (!type)
        continue;
      auto* derived = type->AsDerived();
      if (!derived)
        continue;
      auto* typeSymbol = &derived->typeSymbol();
      if (!typeSymbol)
        continue;

      const auto* details = std::get_if<Fortran::semantics::DerivedTypeDetails>(&typeSymbol->details());
      if (!details)
        continue;

      // derived->HasDefaultInitialization();

      // add edges for finalizers
      for (auto final : details->finals()) {
        edges.emplace_back(Fortran::lower::mangle::mangleName(*functionSymbols.back()),
                           Fortran::lower::mangle::mangleName(*final.second));

        llvm::outs() << "Add edge: " << Fortran::lower::mangle::mangleName(*functionSymbols.back()) << " -> "
                     << Fortran::lower::mangle::mangleName(*final.second) << "\n";
      }
    }
  }

  // following 5 methods are for collecting types and their procedures. see type struct and vector.

  // type def
  bool Pre(const Fortran::parser::DerivedTypeDef&) {
    inDerivedTypeDef = true;
    types.emplace_back();

    return true;
  }
  void Post(const Fortran::parser::DerivedTypeDef&) { inDerivedTypeDef = false; }

  // type stmt like type [, extends(...)] :: body (not exhaustive and not extends)
  void Post(const Fortran::parser::DerivedTypeStmt& t) {
    if (!inDerivedTypeDef)
      return;

    auto& currentType = types.back();
    const auto& name = std::get<Fortran::parser::Name>(t.t);
    currentType.type = name.symbol;
  }

  // type attrs like extends
  void Post(const Fortran::parser::TypeAttrSpec& a) {
    if (!inDerivedTypeDef)
      return;

    auto& currentType = types.back();
    if (std::holds_alternative<Fortran::parser::TypeAttrSpec::Extends>(a.u)) {
      const auto& extends = std::get<Fortran::parser::TypeAttrSpec::Extends>(a.u);
      currentType.extendsFrom = extends.v.symbol;
    }
  }

  // procedures in type defs
  void Post(const Fortran::parser::TypeBoundProcedureStmt& s) {
    if (!inDerivedTypeDef)
      return;

    if (auto* withoutInterface = std::get_if<Fortran::parser::TypeBoundProcedureStmt::WithoutInterface>(&s.u)) {
      for (const auto& d : withoutInterface->declarations) {
        auto& name = std::get<Fortran::parser::Name>(d.t);
        if (!name.symbol)
          return;

        auto& optname = std::get<std::optional<Fortran::parser::Name>>(d.t);
        if (!optname || !optname->symbol) {
          return;
        }

        auto& currentType = types.back();
        currentType.procedures.emplace_back(name.symbol, optname->symbol);
      }

      // only for abstract types, with deferred in binding attr list
    } else if (auto* withInterface = std::get_if<Fortran::parser::TypeBoundProcedureStmt::WithInterface>(&s.u)) {
      for (const auto& n : withInterface->bindingNames) {
        if (!n.symbol)
          return;

        auto& currentType = types.back();
        currentType.procedures.emplace_back(n.symbol, n.symbol);
      }
    }
  }

  // collect defined operators in a type def (operator overloading)
  void Post(const Fortran::parser::TypeBoundGenericStmt& s) {
    if (!inDerivedTypeDef)
      return;

    const auto& genericSpec = std::get<Fortran::common::Indirection<Fortran::parser::GenericSpec>>(s.t);
    if (auto* definedOperator = std::get_if<Fortran::parser::DefinedOperator>(&genericSpec.value().u)) {
      if (auto* intrinsicOp = std::get_if<Fortran::parser::DefinedOperator::IntrinsicOperator>(&definedOperator->u)) {
        const auto& names = std::get<std::list<Fortran::parser::Name>>(s.t);

        auto& currentType = types.back();

        for (auto name : names) {
          if (!name.symbol)
            continue;

          currentType.operators.emplace_back(intrinsicOp, name.symbol);
        }
      }
    }
  }

  // the following 4 methods are for collecting defined operators in interface statements
  bool Pre(const Fortran::parser::InterfaceStmt&) {
    inInterfaceStmt = true;
    return true;
  }
  bool Pre(const Fortran::parser::EndInterfaceStmt&) {
    inInterfaceStmt = false;
    inInterfaceStmtDefinedOperator = false;
    return true;
  }
  void Post(const Fortran::parser::DefinedOperator& op) {
    if (!inInterfaceStmt)
      return;

    inInterfaceStmtDefinedOperator = true;

    interfaceOperators.emplace_back(&op.u, std::vector<Fortran::semantics::Symbol*>());
  }
  void Post(const Fortran::parser::ProcedureStmt& p) {
    if (!inInterfaceStmtDefinedOperator)
      return;

    auto name = std::get<std::list<Fortran::parser::Name>>(p.t);
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

  template <typename Variant, typename... Ts>
  bool holds_any_of(const Variant& v) {
    return (std::holds_alternative<Ts>(v) || ...);
  }

  bool isOperator(const Fortran::parser::Expr* e) {
    using PE = Fortran::parser::Expr;
    return holds_any_of<decltype(e->u), PE::UnaryPlus, PE::Negate, PE::NOT, PE::Power, PE::Multiply, PE::Divide,
                        PE::Add, PE::Subtract, PE::Concat, PE::LT, PE::LE, PE::EQ, PE::NE, PE::GE, PE::GT, PE::AND,
                        PE::OR, PE::EQV, PE::NEQV, PE::DefinedBinary, PE::DefinedUnary>(e->u);
  }

  bool compare_expr_IntrinsicOperator(const Fortran::parser::Expr* expr,
                                      const Fortran::parser::DefinedOperator::IntrinsicOperator* op) {
    if (!expr || !op)
      return false;

    using namespace Fortran::parser;
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

  bool Pre(const Fortran::parser::Expr& e) {
    /* Operators: see 15.4.3.4.2, 10.1.6.1, 6.2.4 (https://j3-fortran.org/doc/year/23/23-007r1.pdf)
       Negate, NOT, Power, Multiply, Divide, Add, Subtract, Concat,
       LT, LE, EQ, NE, GE, GT, AND, OR, EQV, NEQV,
       DefinedUnary, DefinedBinary
     */
    using PE = Fortran::parser::Expr;

    auto* designator = std::get_if<Fortran::common::Indirection<Fortran::parser::Designator>>(&e.u);
    if (designator && !exprStmtWithOps.empty()) {
      auto* dataRef = std::get_if<Fortran::parser::DataRef>(&designator->value().u);
      if (!dataRef)
        return true;

      auto* name = std::get_if<Fortran::parser::Name>(&dataRef->u);
      if (!name || !name->symbol)
        return true;

      auto* type = name->symbol->GetType();
      if (!type)
        return true;

      auto* derived = type->AsDerived();
      if (!derived)
        return true;

      auto* typeSymbol = &derived->typeSymbol();
      if (!typeSymbol)
        return true;

      auto typeWithDerived = find_type_with_derived_types(typeSymbol);

      for (auto e : exprStmtWithOps) {
        // search in derived types
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

        // search in interface operators TODO improve this just add everything in the interface instead of comparing
        // types
        auto it = std::find_if(interfaceOperators.begin(), interfaceOperators.end(), [&](const auto& p) {
          if (auto* intrinsicOp = std::get_if<Fortran::parser::DefinedOperator::IntrinsicOperator>(p.first)) {
            return compare_expr_IntrinsicOperator(e, intrinsicOp);
          }
          if (auto* definedOpName = std::get_if<Fortran::parser::DefinedOpName>(p.first)) {
            llvm::outs() << "definedOpName: " << definedOpName->v.symbol->name() << "\n";

            // TODO: Designator is in another expression

            // auto* designator = std::get_if<Fortran::common::Indirection<Fortran::parser::Designator>>(&e->u);
            // if (!designator)
            //   return false;
            // auto* dataRef = std::get_if<Fortran::parser::DataRef>(&designator->value().u);
            // if (!dataRef)
            //   return false;
            // auto* name = std::get_if<Fortran::parser::Name>(&dataRef->u);
            // if (!name || !name->symbol)
            //   return false;

            // llvm::outs() << "defineOpName: " << definedOpName->v.symbol->name() << " vs " << name->symbol->name()
            //              << "\n";

            // return definedOpName->v.symbol->name() == name->symbol->name();  // doesnt work
            return false;
          }
          return false;
        });
        if (it != interfaceOperators.end()) {
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
      }

      return true;
    }

    if (isOperator(&e)) {
      exprStmtWithOps.emplace_back(&e);
    }

    return true;
  }

  void Post(const Fortran::parser::Expr& e) {
    using PE = Fortran::parser::Expr;

    if (!isOperator(&e)) {
      return;
    }

    if (!exprStmtWithOps.empty()) {
      exprStmtWithOps.pop_back();
    }
  }

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

  std::vector<Fortran::semantics::Symbol*> functionSymbols;
  std::vector<std::vector<const Fortran::parser::Name*>> functionDummyArgs;

  std::vector<type_t> types;

  std::vector<std::pair<
      const std::variant<Fortran::parser::DefinedOpName, Fortran::parser::DefinedOperator::IntrinsicOperator>*,
      std::vector<Fortran::semantics::Symbol*>>>
      interfaceOperators;  // operator name (symbol) => [procedure names (symbols)]

  std::vector<const Fortran::parser::Expr*> exprStmtWithOps;
};

class CollectCG : public Fortran::frontend::PluginParseTreeAction {
 public:
  void executeAction() override {
    auto cg = std::make_unique<metacg::Callgraph>();

    ParseTreeVisitor visitor(cg.get(), getCurrentFile().str());
    Fortran::parser::Walk(getParsing().parseTree(), visitor);

    // add edges
    for (auto edge : visitor.getEdges()) {
      auto* callerNode = cg->getOrInsertNode(edge.first);
      auto* calleeNode = cg->getOrInsertNode(edge.second);
      if (!calleeNode || !callerNode) {
        llvm::outs() << "No nodes found for edge: " << edge.first << " -> " << edge.second << "\n";
        continue;
      }

      cg->addEdge(callerNode, calleeNode);
    }

    auto& mcgManager = metacg::graph::MCGManager::get();

    mcgManager.resetManager();
    mcgManager.addToManagedGraphs("test", std::move(cg), true);
    mcgManager.mergeIntoActiveGraph();

    auto mcgWriter = metacg::io::createWriter(3);
    if (!mcgWriter) {
      llvm::errs() << "Unable to create a writer\n";
      return;
    };

    metacg::io::JsonSink jsonSink;
    mcgWriter->writeActiveGraph(jsonSink);

    auto file = createOutputFile("json");
    file->write(jsonSink.getJson().dump().c_str(), jsonSink.getJson().dump().size());
  }
};

class CollectCGwithDot : public CollectCG {
 public:
  void executeAction() override {
    CollectCG::executeAction();

    auto& mcgManager = metacg::graph::MCGManager::get();

    metacg::io::dot::DotGenerator dotGen(mcgManager.getCallgraph("test"));
    dotGen.generate();

    llvm::SmallString<128> outputPath(getInstance().getFrontendOpts().outputFile);
    if (outputPath.empty()) {
      outputPath = getCurrentFile();
    }
    llvm::sys::path::replace_extension(outputPath, "dot");
    std::error_code ec;
    llvm::raw_fd_ostream out(outputPath.str(), ec);
    if (ec) {
      llvm::errs() << "Error opening output file: " << ec.message() << "\n";
      return;
    }
    out << dotGen.getDotString();
  }
};

static Fortran::frontend::FrontendPluginRegistry::Add<CollectCG> X("genCG", "Generate Callgraph");
static Fortran::frontend::FrontendPluginRegistry::Add<CollectCGwithDot> Y("genCGwithDot", "Generate Callgraph");
