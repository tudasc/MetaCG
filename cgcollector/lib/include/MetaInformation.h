#ifndef CGCOLLECTOR_METAINFORMATION_H
#define CGCOLLECTOR_METAINFORMATION_H

#include "nlohmann/json.hpp"

#include <clang/AST/Expr.h>
#include <iostream>
#include <llvm/ADT/DenseMap.h>

struct MetaInformation {
  virtual void applyOnJSON(nlohmann::json &json, const std::string &functionName, const std::string &metaFieldName,
                           int mcgFormatVersion) = 0;
  virtual bool equals(MetaInformation *mi) = 0;
};

struct NumberOfStatementsResult final : public MetaInformation {
  int numberOfStatements;
  void applyOnJSON(nlohmann::json &json, [[maybe_unused]] const std::string &functionName,
                   const std::string &metaFieldName, int mcgFormatVersion) override {
    if (mcgFormatVersion == 1) {
      json[metaFieldName] = numberOfStatements;
    } else if (mcgFormatVersion == 2) {
      json["meta"][metaFieldName] = numberOfStatements;
    }
  }

  bool equals(MetaInformation *other) override {
    const auto nos = static_cast<NumberOfStatementsResult *>(other);
    return numberOfStatements == nos->numberOfStatements;
  }
};

struct FilePropertyResult final : public MetaInformation {
  std::string fileOrigin;
  bool isFromSystemInclude;
  void applyOnJSON(nlohmann::json &json, [[maybe_unused]] const std::string &functionName,
                   const std::string &metaFieldName, int mcgFormatVersion) override {
    if (mcgFormatVersion > 1) {
      json["meta"][metaFieldName] = {{"origin", fileOrigin}, {"systemInclude", isFromSystemInclude}};
    }
  }

  bool equals(MetaInformation *other) override {
    const auto fpr = static_cast<FilePropertyResult *>(other);
    const auto getFilename = [](auto filepath) {
      auto slashPos = filepath.rfind('/');
      if (slashPos != std::string::npos) {
        auto filename = filepath.substr(slashPos + 1);
        return filename;
      }
      return std::string();
    };
    auto fName = getFilename(fileOrigin);
    auto fNameOther = getFilename(fpr->fileOrigin);

    bool equal = true;
    equal &= (fName == fNameOther);
    equal &= (isFromSystemInclude == fpr->isFromSystemInclude);
    return equal;
  }
};

struct CodeStatisticsMetaInformation : public MetaInformation {
  int numVars;

  void applyOnJSON(nlohmann::json &json, [[maybe_unused]] const std::string &functionName,
                   const std::string &metaFieldName, int mcgFormatVersion) override {
    if (mcgFormatVersion > 1) {
      json["meta"][metaFieldName] = {{"numVars", numVars}};
    }
  }

  bool equals(MetaInformation *other) override {
    const auto csmi = static_cast<CodeStatisticsMetaInformation *>(other);
    return numVars == csmi->numVars;
  }
};

struct MallocVariableInformation : public MetaInformation {
  std::map<std::string, std::string> allocs;
  void applyOnJSON(nlohmann::json &json, [[maybe_unused]] const std::string &functionName,
                   const std::string &metaFieldName, int mcgFormatVersion) override {
    if (mcgFormatVersion > 1) {
      std::vector<nlohmann::json> jArray;
      jArray.reserve(allocs.size());
      for (const auto &[k, v] : allocs) {
        jArray.push_back({{"global", k}, {"allocStmt", v}});
      }
      json["meta"][metaFieldName] = jArray;
    }
  }

  bool equals(MetaInformation *other) override {
    const auto o = static_cast<MallocVariableInformation *>(other);
    return allocs == o->allocs;
  }
};

struct UniqueTypeInformation : public MetaInformation {
  int numTypes{0};

  void applyOnJSON(nlohmann::json &json, [[maybe_unused]] const std::string &functionName,
                   const std::string &metaFieldName, int mcgFormatVersion) override {
    if (mcgFormatVersion > 1) {
      json["meta"][metaFieldName] = numTypes;
    }
  }

  bool equals(MetaInformation *other) override {
    const auto o = static_cast<UniqueTypeInformation *>(other);
    return numTypes == o->numTypes;
  }
};

struct NumOfConditionalBranchesResult final : public MetaInformation {
  void applyOnJSON(nlohmann::json &json, [[maybe_unused]] const std::string &functionName,
                   const std::string &metaFieldName, int mcgFormatVersion) override {
    if (mcgFormatVersion > 1) {
      json["meta"][metaFieldName] = numberOfConditionalBranches;
    }
  }
  bool equals(MetaInformation *mi) override {
    const auto o = static_cast<NumOfConditionalBranchesResult *>(mi);
    return o->numberOfConditionalBranches == numberOfConditionalBranches;
  }

 public:
  int numberOfConditionalBranches;
};

struct NumOperationsResult final : public MetaInformation {
  int numberOfIntOps;
  int numberOfFloatOps;
  int numberOfControlFlowOps;
  int numberOfMemoryAccesses;

  void applyOnJSON(nlohmann::json &json, [[maybe_unused]] const std::string &functionName,
                   const std::string &metaFieldName, int mcgFormatVersion) override {
    if (mcgFormatVersion > 1) {
      json["meta"][metaFieldName] = {{"numberOfIntOps", numberOfIntOps},
                                     {"numberOfFloatOps", numberOfFloatOps},
                                     {"numberOfControlFlowOps", numberOfControlFlowOps},
                                     {"numberOfMemoryAccesses", numberOfMemoryAccesses}};
    }
  }

  bool equals(MetaInformation *mi) override {
    const auto o = static_cast<NumOperationsResult *>(mi);
    return numberOfIntOps == o->numberOfIntOps && numberOfFloatOps == o->numberOfFloatOps &&
           numberOfControlFlowOps == o->numberOfControlFlowOps && numberOfMemoryAccesses == o->numberOfMemoryAccesses;
  };
};

struct LoopDepthResult final : public MetaInformation {
  int loopDepth;

  void applyOnJSON(nlohmann::json &json, [[maybe_unused]] const std::string &functionName,
                   const std::string &metaFieldName, int mcgFormatVersion) override {
    if (mcgFormatVersion > 1) {
      json["meta"][metaFieldName] = loopDepth;
    }
  }
  bool equals(MetaInformation *mi) override {
    const auto o = static_cast<LoopDepthResult *>(mi);
    return o->loopDepth == loopDepth;
  }
};

struct GlobalLoopDepthResult final : public MetaInformation {
  std::map<std::string, int> calledFunctions;

  void applyOnJSON(nlohmann::json &json, [[maybe_unused]] const std::string &functionName,
                   const std::string &metaFieldName, int mcgFormatVersion) override {
    if (mcgFormatVersion > 1) {
      json["meta"][metaFieldName] = calledFunctions;
    }
  }
  bool equals(MetaInformation *mi) override {
    const auto o = static_cast<GlobalLoopDepthResult *>(mi);
    return o->calledFunctions == calledFunctions;
  }
};
#endif /* ifndef CGCOLLECTOR_METAINFORMATION_H */
