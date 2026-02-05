#include "headers.h"

class ParseTreeVisitor {
 public:
  ParseTreeVisitor(metacg::Callgraph* cg, std::string currentFileName) : cg(cg), currentFileName(currentFileName) {};

  template <typename T>
  void handleFuncSubStmt(const T& stmt) {
    if (auto* sym = std::get<Fortran::parser::Name>(stmt.t).symbol) {
      functionNames.emplace_back(Fortran::lower::mangle::mangleName(*sym));
      functionDummyArgs.emplace_back(std::vector<const Fortran::parser::Name*>());
      cg->insert(std::make_unique<metacg::CgNode>(functionNames.back(), currentFileName, false, false));
    }
  }
  void handleEndFuncSubStmt() {
    if (!functionNames.empty()) {
      functionNames.pop_back();
    }
    if (!functionDummyArgs.empty()) {
      functionDummyArgs.pop_back();
    }
  }

  std::vector<std::pair<std::string, std::string>> getEdges() const { return edges; }

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

      functionNames.emplace_back(Fortran::lower::mangle::mangleName(*maybeStmt->statement.v.symbol));
      cg->insert(std::make_unique<metacg::CgNode>(functionNames.back(), currentFileName, false, false));
    }
    return true;
  }

  void Post(const Fortran::parser::MainProgram&) {
    inMainProgram = false;

    if (!functionNames.empty()) {
      functionNames.pop_back();
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

    auto* node = cg->getNode(functionNames.back());
    if (!node) {
      return;
    }

    node->setHasBody(true);
  }

  void Post(const Fortran::parser::FunctionStmt& f) {
    handleFuncSubStmt(f);

    // collect function arguments
    const auto& name_list = std::get<std::list<Fortran::parser::Name>>(f.t);
    for (auto name : name_list) {
      functionDummyArgs.back().push_back(&name);
    }
  }
  void Post(const Fortran::parser::EndFunctionStmt&) { handleEndFuncSubStmt(); }
  void Post(const Fortran::parser::SubroutineStmt& s) {
    handleFuncSubStmt(s);

    // collect subroutine arguments (dummy args)
    const auto* dummyArg_list = &std::get<std::list<Fortran::parser::DummyArg>>(s.t);
    for (const auto& dummyArg : *dummyArg_list) {
      const auto* name = std::get_if<Fortran::parser::Name>(&dummyArg.u);
      functionDummyArgs.back().push_back(name);
    }
  }
  void Post(const Fortran::parser::EndSubroutineStmt&) { handleEndFuncSubStmt(); }

  void Post(const Fortran::parser::ProcedureDesignator& p) {
    if (functionNames.empty())
      return;

    // if just the name is called. (as subroutine with call and as function without call)
    if (auto* name = std::get_if<Fortran::parser::Name>(&p.u)) {
      if (!name->symbol)
        return;

      // ignore intrinsic functions
      if (name->symbol->attrs().test(Fortran::semantics::Attr::INTRINSIC))
        return;

      edges.emplace_back(functionNames.back(), Fortran::lower::mangle::mangleName(*name->symbol));

      // if called from a object with %. (base % component)
    } else if (auto* procCompRef = std::get_if<Fortran::parser::ProcComponentRef>(&p.u)) {
      auto* symbolComp = procCompRef->v.thing.component.symbol;
      if (!symbolComp)
        return;

      edges.emplace_back(functionNames.back(),
                         Fortran::lower::mangle::mangleName(*procCompRef->v.thing.component.symbol));

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

      auto findTypeIt =
          std::find_if(types.begin(), types.end(), [&typeSymbol](const type_t& t) { return t.type == typeSymbol; });
      if (findTypeIt == types.end())
        return;

      // handle base type
      auto baseProcIt = std::find_if(findTypeIt->procedures.begin(), findTypeIt->procedures.end(),
                                     [&symbolComp](const auto& p) { return p.first->name() == symbolComp->name(); });
      if (baseProcIt == findTypeIt->procedures.end())
        return;

      edges.emplace_back(functionNames.back(), Fortran::lower::mangle::mangleName(*baseProcIt->second));

      // handle derived types
      for (const auto& t : types) {
        if (t.extendsFrom != typeSymbol)
          continue;

        auto dProcIt = std::find_if(t.procedures.begin(), t.procedures.end(),
                                    [&symbolComp](const auto& p) { return p.first->name() == symbolComp->name(); });
        if (dProcIt == t.procedures.end())
          continue;

        edges.emplace_back(functionNames.back(), Fortran::lower::mangle::mangleName(*dProcIt->second));
      }
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
        edges.emplace_back(functionNames.back(), Fortran::lower::mangle::mangleName(*final.second));
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

  template <typename Variant, typename... Ts>
  bool holds_any_of(const Variant& v) {
    return (std::holds_alternative<Ts>(v) || ...);
  }

  void Post(const Fortran::parser::Expr& e) {
    /* Operators: see 15.4.3.4.2, 10.1.6.1, 6.2.4 (https://j3-fortran.org/doc/year/23/23-007r1.pdf)
      Negate, NOT, Power, Multiply, Divide, Add, Subtract, Concat,
      LT, LE, EQ, NE, GE, GT, AND, OR, EQV, NEQV,
      DefinedUnary, DefinedBinary
    */
    using PE = Fortran::parser::Expr;

    if (holds_any_of<decltype(e.u), PE::DefinedUnary, PE::DefinedBinary>(e.u)) {
      if (const auto* definedUnary = std::get_if<PE::DefinedUnary>(&e.u)) {
        const auto& opname = std::get<Fortran::parser::DefinedOpName>(definedUnary->t);
        if (!opname.v.symbol)
          return;
      }
      if (const auto* definedBinary = std::get_if<PE::DefinedBinary>(&e.u)) {
        const auto& opname = std::get<Fortran::parser::DefinedOpName>(definedBinary->t);
        if (!opname.v.symbol)
          return;
      }
    }

    if (holds_any_of<decltype(e.u), PE::Negate, PE::NOT, PE::Power, PE::Multiply, PE::Divide, PE::Add, PE::Subtract,
                     PE::Concat, PE::LT, PE::LE, PE::EQ, PE::NE, PE::GE, PE::GT, PE::AND, PE::OR, PE::EQV, PE::NEQV>(
            e.u)) {
      std::visit(
          [&](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            const Fortran::common::Indirection<Fortran::parser::Expr>* expr = nullptr;
            if constexpr (std::is_base_of_v<PE::IntrinsicBinary, T>) {
              expr = &std::get<0>(arg.t);
            } else if constexpr (std::is_base_of_v<PE::IntrinsicUnary, T>) {
              expr = &arg.v;
            } else {
              return;  // not a unary or binary operator
            }

            auto* designator = std::get_if<Fortran::common::Indirection<Fortran::parser::Designator>>(&expr->value().u);
            if (!designator)
              return;

            auto* dataRef = std::get_if<Fortran::parser::DataRef>(&designator->value().u);
            if (!dataRef)
              return;

            auto* name = std::get_if<Fortran::parser::Name>(&dataRef->u);
            if (!name || !name->symbol)
              return;

            auto* type = name->symbol->GetType();
            if (!type)
              return;

            auto* derived = type->AsDerived();
            if (!derived)
              return;

            auto* typeSymbol = &derived->typeSymbol();
            if (!typeSymbol)
              return;

            auto findTypeIt =
                std::find_if(types.begin(), types.end(), [=](const type_t& t) { return t.type == typeSymbol; });
            if (findTypeIt == types.end())
              return;

            for (const auto& ops : findTypeIt->operators) {
              auto* op = ops.first;
              auto* sym = ops.second;

              auto baseProcIt = std::find_if(findTypeIt->procedures.begin(), findTypeIt->procedures.end(),
                                             [&](const auto& p) { return p.first->name() == sym->name(); });
              if (baseProcIt != findTypeIt->procedures.end()) {
                edges.emplace_back(functionNames.back(), Fortran::lower::mangle::mangleName(*baseProcIt->second));
              }

              for (const auto& t : types) {
                if (t.extendsFrom != typeSymbol)
                  continue;
                auto dProcIt = std::find_if(t.procedures.begin(), t.procedures.end(),
                                            [&](const auto& p) { return p.first->name() == sym->name(); });
                if (dProcIt != t.procedures.end()) {
                  edges.emplace_back(functionNames.back(), Fortran::lower::mangle::mangleName(*dProcIt->second));
                }
              }
            }
          },
          e.u);
    }
  }

 private:
  metacg::Callgraph* cg;
  std::vector<std::pair<std::string, std::string>> edges;  // (caller, callee)
  std::string currentFileName;

  std::vector<std::string> functionNames;
  std::vector<std::vector<const Fortran::parser::Name*>> functionDummyArgs;
  bool inFunctionOrSubroutineSubProgram = false;
  bool inMainProgram = false;
  bool inDerivedTypeDef = false;

  typedef struct type {
    Fortran::semantics::Symbol* type;
    Fortran::semantics::Symbol* extendsFrom;
    std::vector<std::pair<Fortran::semantics::Symbol*, Fortran::semantics::Symbol*>>
        procedures;  // name(symbol) => optname(symbol)
    std::vector<std::pair<const Fortran::parser::DefinedOperator::IntrinsicOperator*, Fortran::semantics::Symbol*>>
        operators;  // operator => name(symbol) TODO: do i need IntrinsicOperator
  } type_t;
  std::vector<type> types;
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
