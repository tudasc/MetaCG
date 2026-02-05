#include "headers.h"

class ParseTreeVisitor {
 public:
  ParseTreeVisitor(metacg::Callgraph* cg, std::string currentFileName) : cg(cg), currentFileName(currentFileName) {};

  template <typename T>
  void handleFuncSubStmt(const T& stmt) {
    if (auto* sym = std::get<Fortran::parser::Name>(stmt.t).symbol) {
      functionNames.emplace_back(Fortran::lower::mangle::mangleName(*sym));
      cg->insert(std::make_unique<metacg::CgNode>(functionNames.back(), currentFileName, false, false));
    }
  }
  void handleEndFuncSubStmt() {
    if (!functionNames.empty()) {
      functionNames.pop_back();
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

  void Post(const Fortran::parser::FunctionStmt& f) { handleFuncSubStmt(f); }
  void Post(const Fortran::parser::EndFunctionStmt&) { handleEndFuncSubStmt(); }
  void Post(const Fortran::parser::SubroutineStmt& s) { handleFuncSubStmt(s); }
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

        auto dProcIt = std::find_if(t.procedures.begin(), t.procedures.end(), [&symbolComp](const auto& p) {
          return p.first->name() == symbolComp->name();
        });  // TODO: use statement
        if (dProcIt == t.procedures.end())
          continue;

        edges.emplace_back(functionNames.back(), Fortran::lower::mangle::mangleName(*dProcIt->second));
      }
    }
  }

  // TODO: handle destructors (finalizers)
  void Post(const Fortran::parser::TypeDeclarationStmt& t) {
    // TODO they dont need to hold intent
    // const auto& attrs = std::get<std::list<Fortran::parser::AttrSpec>>(t.t);
    // for (const auto& attr : attrs) {
    //   if (std::holds_alternative<Fortran::parser::IntentSpec>(attr.u)) {
    //     return;
    //   }
    // }

    // const auto& entityDecls = std::get<std::list<Fortran::parser::EntityDecl>>(t.t);
    // for (const auto& entity : entityDecls) {
    //   const auto& name = std::get<Fortran::parser::ObjectName>(entity.t);
    //   if (!name.symbol)
    //     continue;

    //   // TODO they dont need to hold intent
    //   if (name.symbol->attrs().test(Fortran::semantics::Attr::INTENT_IN))
    //     continue;  // skip intent in

    //   llvm::outs() << "Found type declaration: " << name.symbol->name().ToString() << "\n";
    //   if (auto* type = name.symbol->GetType()) {
    //     if (auto* derived = type->AsDerived()) {
    //       if (derived->HasDestruction()) {
    //         llvm::outs() << "Found derived type with destruction: " << name.symbol->name().ToString() << "\n";
    //       }
    //     }
    //   }
    // }
  }

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
  void Post(const Fortran::parser::TypeBoundProcDecl& d) {
    if (!inDerivedTypeDef)
      return;

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

 private:
  metacg::Callgraph* cg;
  std::vector<std::pair<std::string, std::string>> edges;  // (caller, callee)
  std::string currentFileName;

  std::vector<std::string> functionNames;
  bool inFunctionOrSubroutineSubProgram = false;
  bool inMainProgram = false;
  bool inDerivedTypeDef = false;

  typedef struct type {
    Fortran::semantics::Symbol* type;
    Fortran::semantics::Symbol* extendsFrom;
    std::vector<std::pair<Fortran::semantics::Symbol*, Fortran::semantics::Symbol*>> procedures;
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
