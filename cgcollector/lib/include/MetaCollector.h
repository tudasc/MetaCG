#ifndef CGCOLLECTOR_METACOLLECTOR_H
#define CGCOLLECTOR_METACOLLECTOR_H

#include "helper/ASTHelper.h"
#include "helper/common.h"

#include "CallGraph.h"

#include "nlohmann/json.hpp"

#include <clang/AST/Decl.h>

#include <iostream>
#include <memory>
#include <unordered_map>

class MetaInformation {
  // std::string name;

  // protected:
  // MetaInformation(std::string name) : name(name) {}

 public:
  virtual void applyOnJSON(nlohmann::json &json, const std::string &functionName, const std::string &metaFieldName) = 0;

  // std::string getName() {
  //  return name;
  //}
};

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

class NumberOfStatementsResult final : public MetaInformation {
 public:
  int numberOfStatements;

  void applyOnJSON(nlohmann::json &json, const std::string &functionName, const std::string &metaFieldName) override {
    json[functionName][metaFieldName] = numberOfStatements;
  };

  // NumberOfStatementsResult() : MetaInformation("NumberOfStatements") {}
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

#endif
