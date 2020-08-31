#ifndef CGCOLLECTOR_METACOLLECTOR_H
#define CGCOLLECTOR_METACOLLECTOR_H

#include "helper/ASTHelper.h"
#include "helper/common.h"

#include "CallGraph.h"
#include "MetaInformation.h"

#include <clang/AST/Decl.h>

#include <memory>
#include <unordered_map>

class MetaCollector {
  std::string name;

  std::unordered_map<std::string, std::unique_ptr<MetaInformation>> values;

  virtual std::unique_ptr<MetaInformation> calculateForFunctionDecl(clang::FunctionDecl const *const decl) = 0;

 protected:
  MetaCollector(std::string name) : name(name) {}

 public:
  void calculateFor(const CallGraph &cg) {
    for (auto &node : cg) {
      if (node.first) {
        if (auto f = llvm::dyn_cast<clang::FunctionDecl>(node.first)) {
          auto names = getMangledName(f);
          for (const auto n : names) {
            values[n] = calculateForFunctionDecl(f);
          }
        }
      }
    }
  }

  const std::unordered_map<std::string, std::unique_ptr<MetaInformation>> &getMetaInformation() { return values; }

  std::string getName() { return name; }
};

class NumberOfStatementsCollector final : public MetaCollector {
  std::unique_ptr<MetaInformation> calculateForFunctionDecl(clang::FunctionDecl const *const decl) override {
    std::unique_ptr<NumberOfStatementsResult> result = std::make_unique<NumberOfStatementsResult>();

    result->numberOfStatements = getNumStmtsInStmt(decl->getBody());

    return result;
  }

 public:
  NumberOfStatementsCollector() : MetaCollector("numStatements") {}
};

#endif /* ifndef CGCOLLECTOR_METACOLLECTOR_H */
