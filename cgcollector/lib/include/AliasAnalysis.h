/**
 * File: AliasAnalysis.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#ifndef CGCOLLECTOR_ALIASANALYSIS_H
#define CGCOLLECTOR_ALIASANALYSIS_H

#include "CallGraph.h"

#include "nlohmann/json.hpp"

#include <clang/AST/Decl.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <llvm/ADT/SmallString.h>

#include <algorithm>
#include <iterator>
#include <list>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <unordered_set>
#include <vector>

#if !defined(DEBUG_TEST_AA) || defined(NDEBUG)
template <typename T>
using ListType = std::list<T>;
template <typename K, typename V>
using MapType = std::map<K, V>;
template <typename T>
using SetType = std::set<T>;
template <typename T>
using VectorType = std::vector<T>;
using StringType = std::string;

#else

#include <debug/list>
#include <debug/map>
#include <debug/set>
#include <debug/string>
#include <debug/vector>

template <typename T>
using ListType = __gnu_debug::list<T>;
template <typename K, typename V>
using MapType = __gnu_debug::map<K, V>;
template <typename T>
using SetType = __gnu_debug::set<T>;
template <typename T>
using VectorType = __gnu_debug::vector<T>;
using StringType = __gnu_debug::string;

#endif

/* Implementation TODO:
 * Currently a sorted vector is quite often used instead of a set. This makes sense if there are first insertions and
 * then queries. Maybe change this if the access patterns changes, if this is a bottleneck
 *
 * For easier debug and a consistent serialisation, as well as stable iterators in some cases im using std::map in all
 * cases. If performance becomes more important this could be replaced by llvm::StringMap llvm::DenseMap
 *
 * A cash to store functions that are already merged would reduce some redundant lookups / merge attempts
 *
 * It currently does not work with the global loop depth
 *
 * Calls in function templates generate the same id
 *
 */

/**
 *  This function calculates alias information used to resolve pointer calls
 *  It is based on the ideas / algorithms presented in:
 *  Milanova, Ana, Atanas Rountev, and Barbara G. Ryder. Precise call graphs for C programs with function pointers.
 *  https://doi.org/10.1023/b:ause.0000008666.56394.a1
 *  Zhang, Xiang-Xiang Sean. Practical pointer aliasing analysis.
 *  https://doi.org/10.7282/T3862M19
 *
 *  Supported Constructs:
 *  Standard C constructs: Testing
 *  Weird nested structs : Testing, accessing them should work, but the initialisation may have some unhandled edge
 * cases
 * Calls not in functions: TODO Constructors for static variables can run outside of a function
 * Default parameters: Testing We do not walk the default arguments as they are written in the function declaration,
 *  instead we just walk the CXXDefaultArgExpr when the function gets called
 * MTU: Differing default arguments: Done
 * MTU: Unnamed arguments: Done
 * C Style struct assing init: Done
 * Smart handling of pointer arithmetics: Testing
 * void *: Done
 *  __builtin functions : Done
 *  Functions that are called without declaring them before: Testing, we do not have a functiondecl for them so not
 *      sure yet how to handle that, but its undefined behaviour anyway. Used by AMG
 * Variadic functions: Half Done, variadic functions are supported, but the variadic argument itself is not
 *  handled
 * Inheritance: Testing
 * this pointer: Testing
 * Constructors etc: Done  Move and copy constructor are a bit confusing, as they only assign the members in a class,
 *  but not the classes themselves
 * overloaded functions: Testing
 * Member functions: Testing
 *  The plan for C++ classes is the following: Each member expression gets handled as if it is a freestanding function,
 * with this as a secret first parameter When we see a call to a member function, we insert the object on which it is
 * called as that first parameter
 * Overloaded functions are getting handled by merging their parameters, this pointer and return value together
 * memcpy etc: TODO May need some help if its implemented as a compiler builtin
 * C++ references: Done
 * Lambda functions: Testing
 * Templates: Testing
 * Multi Translation Unit Support: Testing
 * std::function: TODO
 * std::bind (std::tuple): TODO
 * unions: Testing
 * C++ casting: Testing, dynamic_cast may need some special attention
 * ExprWithCleanups: Done We don't need to do anything here
 * MaterializeTempEpxr: Testing We don't need to do anything here, but its important to know that other objects can bind
 *  to this
 * CXXBindTemporaryExpr: Testing Call the destructor of the temporary,
 *  https://github.com/llvm/llvm-project/blob/llvmorg-10.0.1/clang/lib/Sema/SemaExpr.cpp#L17162
 * CXXTemporaryObjectExpr: Testing Call The constructor + return object to bind too
 * CXXFunctionalCastExpr: Testing We don't need to do anything here. The clang documentation seems to be a bit wrong
 * here, it says that CXXTemporaryObjectExpr with exactly 1 argument are CXXFunctionalCastExpr, but this seems to not be
 * true for custom classes
 * {} return: Testing Call the constructor
 * CXXDefaultInitExpr: Testing
 * New, Delete: TESTING
 * C++ Constructor Variable init: Done
 * C++ Constructor Base Class init: Done (Talk with JP about implicit constructors)
 * C++ Operators: Done
 * Custom Operator New / Delete (including arrays, placement new etc) : TODO (placement new has a hacky solution)
 * C++ Conversation Operators: Testing
 * C++ exceptions: I think we can safely assume that nobody is throwing function pointers
 * Virtual Destructors: Testing
 * CXXPseudoDestructorExpr: Done
 * TypeId: TODO
 * clang::ArrayInitLoopExpr: TODO, for example in <locale>
 *
 * @param TD
 */
void calculateAliasInfo(clang::TranslationUnitDecl *TD, CallGraph *CG, nlohmann::json &J, int MetacgFormatVersion,
                        bool captureCtorsDtors, bool captureStackCtorsDtors);

namespace implementation {

struct ObjectNameMember;
struct ObjectNameDereference;

struct ObjectName {
  explicit ObjectName(clang::NamedDecl *Decl);
  ObjectName(clang::CallExpr *CE, const clang::ASTContext *CTX, clang::FunctionDecl *ParentFunctionDecl);
  ObjectName(clang::CXXConstructExpr *CE, const clang::ASTContext *CTX, clang::FunctionDecl *ParentFunctionDecl);
  ObjectName(const clang::CXXThisExpr *, clang::FunctionDecl *ParentFunctionDecl);
  // TODO: TypeId
  // ObjectName(const clang::CXXTypeidExpr *TE);
  ObjectName(clang::CXXNewExpr *NE, clang::FunctionDecl *ParentFunctionDecl);
  explicit ObjectName(StringType Name);
  ObjectName() = default;
  ObjectName(clang::MaterializeTemporaryExpr *MTE, clang::FunctionDecl *ParentFunctionDecl);
  StringType Object;
  StringType getStringRepr() const;
  bool operator<(const ObjectName &rhs) const;
  bool operator>(const ObjectName &rhs) const;
  bool operator<=(const ObjectName &rhs) const;
  bool operator>=(const ObjectName &rhs) const;
  bool operator==(const ObjectName &rhs) const;
  bool operator!=(const ObjectName &rhs) const;
  /**
   * Return null if it is not a function
   * @return
   */
  std::optional<StringType> GetFunctionName() const;
  bool Function = false;
};

/**
 * Describes the reference / dereference level.
 * 0 means a normal variable,
 * 1 means *
 * 2 means **
 * -1 means &
 * -2 means && and so on
 */
struct ObjectNameDereference {
  // std::variant would be better here, but it is quite cumbersome to use
 private:
  std::shared_ptr<ObjectName> OB;
  std::shared_ptr<ObjectNameMember> MB;

 public:
  ObjectNameDereference(std::shared_ptr<ObjectName> OB, short DereferenceLevel = 0);
  ObjectNameDereference(std::shared_ptr<ObjectNameMember> MB, short DereferenceLevel = 0);
  ObjectNameDereference() = default;

  inline ObjectName *getOb() const { return OB.get(); }
  inline void setOb(std::shared_ptr<ObjectName> Ob) { OB = std::move(Ob); }
  inline ObjectNameMember *getMb() const { return MB.get(); }
  inline void setMb(std::shared_ptr<ObjectNameMember> Mb) { MB = std::move(Mb); }

  // See class documentation
  int DereferenceLevel = 0;

  StringType GetStringRepr() const;
  bool operator==(const ObjectNameDereference &rhs) const;
  bool operator!=(const ObjectNameDereference &rhs) const;
  bool operator<(const ObjectNameDereference &rhs) const;
  bool operator>(const ObjectNameDereference &rhs) const;
  bool operator<=(const ObjectNameDereference &rhs) const;
  bool operator>=(const ObjectNameDereference &rhs) const;
  /**
   * Return null if it is not a function
   * @return
   */
  std::optional<StringType> GetFunctionName() const;
};

struct ObjectNameMember {
  ObjectNameMember() = default;
  ObjectNameMember(ObjectNameDereference DB, StringType Member);

  ObjectNameDereference DB;
  StringType Member;
  StringType GetStringRepr() const;
  bool operator==(const ObjectNameMember &rhs) const;
  bool operator!=(const ObjectNameMember &rhs) const;
  bool operator<(const ObjectNameMember &rhs) const;
  bool operator>(const ObjectNameMember &rhs) const;
  bool operator<=(const ObjectNameMember &rhs) const;
  bool operator>=(const ObjectNameMember &rhs) const;
};

struct Prefix {
  Prefix() = default;
  Prefix(const ObjectNameDereference &Object);
  Prefix(const ObjectNameDereference &Object, StringType Member);
  Prefix(StringType Object, StringType Member);
  bool operator==(const Prefix &rhs) const;
  bool operator!=(const Prefix &rhs) const;
  StringType Object;
  /**
   * empty means this is a * operator
   */
  StringType Member;
  StringType GetStringRepr() const;
  bool operator<(const Prefix &rhs) const;
  bool operator>(const Prefix &rhs) const;
  bool operator<=(const Prefix &rhs) const;
  bool operator>=(const Prefix &rhs) const;
};

struct EquivClass {
  EquivClass() = default;
  /**
   * Convenience Constructor to create a single element EquivClass
   * @param Obj
   */
  EquivClass(StringType Obj);
  VectorType<StringType> Objects;
  VectorType<Prefix> Prefixes;
};
using EquivClassesIterator = ListType<EquivClass>::iterator;

struct FunctionInfo {
  FunctionInfo() = default;
  /**
   *
   * @param FD The function decl to generate the info for
   * @param RealLambdaMethod If FD is a lambda static invoker, this needs to point to the lambda call operator
   */
  explicit FunctionInfo(clang::FunctionDecl *FD, clang::CXXMethodDecl *RealLambdaMethod = nullptr);
  std::vector<std::string> MangledNames;
  VectorType<StringType> Parameters;
  /**
   * This contains all variables that are referenced in returns as well as an symbolic return value
   * The symbolic return value is needed so that we can merge the returns of virtual functions, even if one of them does
   * not reference any variable in its return statements
   */
  VectorType<StringType> ReferencedInReturnStmts;
  bool IsVariadic = false;
};

struct CallInfo {
  CallInfo() = default;
  CallInfo(clang::CallExpr *CE, const clang::ASTContext *CTX, clang::FunctionDecl *ParentFunctionDecl,
           bool IsOperatorMember);
  CallInfo(clang::CXXConstructExpr *CE, const clang::ASTContext *CTX, clang::FunctionDecl *ParentFunctionDecl);
  VectorType<StringType> CalledObjects;
  VectorType<VectorType<StringType>> Arguments;
};

using CallInfoConstIterType = MapType<StringType, CallInfo>::const_iterator;

/**
 * This struct contains all data that is required for serialization and deserialization of equivalence classes,
 * including prefixes, Function and Call Info. FindMap and ReferencedInCalls are not serialized but recreated
 */
struct EquivClassContainer {
  EquivClassContainer() = default;

  ListType<EquivClass> EquivClasses;  // std::list for stable iterators

  MapType<StringType, StringType> FunctionMap;  // Maps an equivalence class to a function name if it is one
  // map to keep element order consistent
  MapType<StringType, FunctionInfo>
      FunctionInfoMap;                        // Maps the string identifier of a function to information about it
  MapType<StringType, CallInfo> CallInfoMap;  // Maps the string identifier of a call to its call information
  MapType<StringType, EquivClassesIterator> FindMap;
  MapType<StringType, StringType> CallExprParentMap;  // Maps each call expr (Key) to the function containing it

  MapType<StringType, VectorType<CallInfoConstIterType>>
      ReferencedInCalls;  // Maps Objects to the calls which call them

  /**
   * Walks true all calls and finds the referenced callee (this can be an unresolved pointer).
   */
  void InitReferencedInCalls();
};

/**
 * Merges the two Equivalence Classes pointed to by E1 and E2
 * @param Data
 * @param E1
 * @param E2
 * @return
 * Classes
 */
std::pair<VectorType<std::pair<StringType, CallInfoConstIterType>>, EquivClassesIterator> mergeRecurisve(
    EquivClassContainer &Data, EquivClassesIterator E1, EquivClassesIterator E2);

struct MergeResult {
  MapType<StringType, EquivClassesIterator>::const_iterator Iter;
  VectorType<std::pair<StringType, StringType>> ObjectsToMerge;
};

MergeResult merge(EquivClassContainer &Data, EquivClassesIterator E1, EquivClassesIterator E2);

/**
 *
 * @param Data
 * @param Ret All function merges resulting from the creation of class E
 * @param E
 */
void GetFunctionsToMerge(const EquivClassContainer &Data, VectorType<std::pair<StringType, CallInfoConstIterType>> &Ret,
                         const EquivClass &E);

/**
 *
 * @param Data
 * @param CalledObj
 * @param CE
 * @return If the merge was done, functions to merge as a result of it, the name of the calling function (first) and of
 * the called function (second)
 */
std::optional<std::pair<VectorType<std::pair<StringType, CallInfoConstIterType>>, std::pair<StringType, StringType>>>
mergeFunctionCall(EquivClassContainer &Data, const StringType &CalledObj, CallInfoConstIterType CE);

std::optional<std::pair<VectorType<std::pair<StringType, CallInfoConstIterType>>, std::pair<StringType, StringType>>>
mergeFunctionCallImpl(EquivClassContainer &Data, const StringType &CalledFunctionName,
                      const StringType &CallingFunctionName, const CallInfo &CI, const StringType &CallExprUSR);

void GetPrefixesToMerge(EquivClass &EquivClassToMergeInto, const VectorType<Prefix> &PrefixToMerge,
                        VectorType<std::pair<StringType, StringType>> &ObjectsToMerge);

/**
 * Class to extract information required by the alias analysis from the given AST
 * This information is the following:
 * - All object names, where objects are defined as memory locations and addresses of memory locations
 * - All assignment operations, with information about the referenced objects
 * - All return operations with their referenced objects
 * - Call operations
 */
class ASTInformationExtractor : public clang::RecursiveASTVisitor<ASTInformationExtractor> {
 public:
  ASTInformationExtractor(const clang::ASTContext *CTX, CallGraph *CG, bool CaptureCtorsDtors,
                          bool CaptureStackCtorsDtors);
  bool shouldVisitTemplateInstantiations() const { return true; }
  bool shouldWalkTypesOfTypeLocs() const { return TraverseTypesOfTypeLocs; }
  bool shouldVisitImplicitCode() const { return true; }

  /**
   * Helper function to avoid code duplication for TraverseFunctionDecl, TraverseCXXMethodDecl and similar
   * @tparam DeclTT
   * @tparam FT
   * @param Decl
   * @param TraverseFunction
   * @return
   */
  template <typename DeclT, typename FT>
  bool TraverseFunctionLikeHelper(DeclT Decl, FT TraverseFunction) {
    // Skip templates
    if (Decl->isDependentContext()) {
      return true;
    }
    const bool PreviousShouldVisitType = TraverseTypeLocs;
    TraverseTypeLocs = true;
    const auto PreviousFunctionDecl = CurrentFunctionDecl;
    CurrentFunctionDecl = Decl;
    auto Ret = (this->*TraverseFunction)(Decl);
    // The RecursiveASTVisitor does for some reason not visit the parameters of lambda invoke operations, instead it
    // visits the parameters of the call operator. I am not sure if this is caused by the RecursiveASTVisitor itself or
    // if it is a bug caused by how we are using it. The workaround for this is to traverse the parameters of the invoke
    // functions manually
    if constexpr (std::is_same_v<std::remove_cv_t<DeclT>, clang::CXXMethodDecl *>) {
      if (Decl->isLambdaStaticInvoker()) {
        for (auto Param : Decl->parameters()) {
          Ret &= TraverseParmVarDecl(Param);
        }
      }
    }
    for (auto Param : Decl->parameters()) {
      // See test 0243
      if (Param->isImplicit()) {
        Ret &= TraverseParmVarDecl(Param);
      }
    }
    CurrentFunctionDecl = PreviousFunctionDecl;
    TraverseTypeLocs = PreviousShouldVisitType;
    return Ret;
  }

  bool VisitFunctionDecl(clang::FunctionDecl *FD);
  bool VisitVarDecl(clang::VarDecl *VD);
  bool VisitFieldDecl(clang::FieldDecl *FD);
  // There does not exist a visit method for CXXCtorInitializers
  bool TraverseConstructorInitializer(clang::CXXCtorInitializer *CtorInit);
  bool VisitCompoundAssignOperator(clang::CompoundAssignOperator *CAO);

  // These function only exists in clang versions less than 11
#if LLVM_VERSION_MAJOR < 11
  bool VisitBinAssign(clang::BinaryOperator *BO);
#else
  bool VisitBinaryOperator(clang::BinaryOperator *BO);
#endif

  bool VisitCallExpr(clang::CallExpr *CE);
  void ConstructExprHelper(clang::CXXConstructExpr *CE, ObjectNameDereference ThisDeref);
  void ConstructExprHelper(clang::CXXConstructExpr *CE, StringType ThisDeref);
  bool VisitMemberExpr(clang::MemberExpr *ME);

  // These functions only exists in clang versions less than 11
#if LLVM_VERSION_MAJOR < 11
  bool TraverseUnaryDeref(clang::UnaryOperator *UO, DataRecursionQueue *Queue = nullptr);
  bool TraverseUnaryAddrOf(clang::UnaryOperator *UO, DataRecursionQueue *Queue = nullptr);
#else
  bool TraverseUnaryOperator(clang::UnaryOperator *UO, DataRecursionQueue *Queue = nullptr);
#endif

  bool VisitDeclRefExpr(clang::DeclRefExpr *DE);
  bool VisitCXXThisExpr(clang::CXXThisExpr *TE);
  bool VisitCXXNewExpr(clang::CXXNewExpr *NE);
  bool VisitCXXDeleteExpr(clang::CXXDeleteExpr *DE);
  bool TraverseArraySubscriptExpr(clang::ArraySubscriptExpr *Expr, DataRecursionQueue *Queue = nullptr);
  bool TraverseBinaryOperator(clang::BinaryOperator *BO, DataRecursionQueue *Queue = nullptr);

  bool TraverseFunctionDecl(clang::FunctionDecl *FD);
  bool TraverseCXXDeductionGuideDecl(clang::CXXDeductionGuideDecl *DD);
  bool TraverseCXXMethodDecl(clang::CXXMethodDecl *MD);
  bool TraverseCXXConstructorDecl(clang::CXXConstructorDecl *CD);
  bool TraverseCXXConversionDecl(clang::CXXConversionDecl *CD);
  bool TraverseCXXDestructorDecl(clang::CXXDestructorDecl *DD);

  bool TraverseCXXNewExpr(clang::CXXNewExpr *NE, [[maybe_unused]] DataRecursionQueue *Queue = nullptr);
  bool VisitCXXConstructExpr(clang::CXXConstructExpr *CE);
  bool VisitMaterializeTemporaryExpr(clang::MaterializeTemporaryExpr *MTE);
  // Destruction of Temporary expressions is handled here
  bool VisitCXXBindTemporaryExpr(clang::CXXBindTemporaryExpr *BTE);

  // Skip code that we are not interested in
  bool TraverseStaticAssertDecl(clang::StaticAssertDecl *SAD);
  bool TraverseDeclarationNameInfo(clang::DeclarationNameInfo);
  bool TraverseCXXNoexceptExpr(clang::CXXNoexceptExpr *, [[maybe_unused]] DataRecursionQueue *Queue = nullptr);

  // Extracted from the recursive ast visitor because by default it always visits noexcept specifiers
  bool TraverseFunctionProtoTypeLoc(clang::FunctionProtoTypeLoc TL);

  /**
   * We only use this to prevent traversing of types in  a function body
   * @param CS
   * @return
   */
  bool TraverseCompoundStmt(clang::CompoundStmt *CS);

  bool TraverseTypedefDecl(clang::TypedefDecl *TDD);
  bool TraverseTypeAliasDecl(clang::TypeAliasDecl *);
  bool TraverseParmVarDecl(clang::ParmVarDecl *PD);

  bool VisitRecordType(clang::RecordType *RT);

  // bool TraverseTypeDecl(clang::TypeDecl *TD);
  bool TraverseTypeLoc(clang::TypeLoc TL);
  bool TraverseRecordDecl(clang::RecordDecl *RD);
  bool TraverseCXXRecordDecl(clang::CXXRecordDecl *RD);

  // Skip template parameters
  bool TraverseTemplateTypeParmDecl(clang::TemplateTypeParmDecl *) { return true; }
  bool TraverseNonTypeTemplateParmDecl(clang::NonTypeTemplateParmDecl *) { return true; }

  void calculatePERelation();

  [[maybe_unused]] void dump();
  const EquivClassContainer &getData() const;

 private:
  const clang::ASTContext *CTX;
  CallGraph *CG;
  bool CaptureCtorsDtors;
  bool CaptureStackCtorsDtors;

  bool TraverseTypeLocs = false;
  bool TraverseTypesOfTypeLocs = false;

  int DereferenceLevel = 0;

  bool InTypededDecl = false;
  bool InNew = false;

  clang::FunctionDecl *CurrentFunctionDecl = nullptr;
  const clang::RecordType *CurrentRecordType = nullptr;

  SetType<ObjectNameDereference> Objects;

  struct InitInfo {
    InitInfo(const ObjectNameDereference &object, clang::Expr *init, clang::FunctionDecl *parentFunction);
    ObjectNameDereference Object;
    clang::Expr *Init;
    clang::FunctionDecl *ParentFunction;
  };

  VectorType<InitInfo> Initalizers;
  VectorType<std::pair<clang::BinaryOperator *, clang::FunctionDecl *>> Assignments;
  MapType<StringType, clang::FunctionDecl *> FuncDeclMap;  // Maps each function to its clang decl
  VectorType<std::pair<StringType, VectorType<StringType>>>
      ConstructorMergeList;  // List of merges that need to happen because of a constructor, first is lhs, second are
                             // possible rhs

  // List containing C++ class methods and members to be merged together
  VectorType<std::pair<StringType, StringType>> CXXMemberMergeList;

  // List containing 'this' pointers to merge. First is the implicit object, second is the method getting called
  VectorType<std::pair<StringType, StringType>> ThisPointersToMerge;

  // List containing everything that needs to be merged as the result of merging overriden functions
  VectorType<std::pair<StringType, StringType>> OverridesToMerge;

  // List containing direct calls in the code, for example construct expressions. second is the parent function, third
  // is the callexpr USR, fourth is the called function USR
  VectorType<std::tuple<CallInfo, clang::FunctionDecl *, StringType, StringType>> DirectCalls;

  /**
   * Sets up the data structures for the equivalence classes
   * Assumes that 'Objects' does not contain duplicates
   */
  void initEquivClasses();

  /**
   * Sets up the prefix classes
   * Assumes that 'initEquivClass' has already been called
   */
  void initPrefixClasses();

  /**
   * Handle merges of all assignments. (=)
   */
  void handleAssignments();

  /**
   * Handle variable inits and merges resulting from them.
   */
  void handleInits();
  void handleInitExpr(const ObjectNameDereference *ObjDeref, clang::Stmt *InitExpr, const clang::Type *VarType,
                      clang::FunctionDecl *ParentFunctionDecl);
  const clang::Type *stripArrayType(const clang::Type *Type) const;

  /**
   * Handle merges resulting from assigning of constructed objects to a variable or similar
   */
  void handleMergeConstructorAssignments();

  /**
   * Handle direct calls
   */
  void handleDirectCalls();

  /**
   * Handle implicit destructor calls, when a class goes out of scope
   */
  void handleDestructorCalls();

  /**
   * Merges the parameters of functions with multiple declarations / definitions
   */
  void handleMergeFunctionParameters();

  /**
   * Merges C++ Member Methods which refer to the same function
   */
  void handleMergeCXXMembers();

  /**
   * Merges the implicit calling object into the this pointer of the called method
   */
  void handleMergeCXXThisPointers();

  /**
   * Merges this pointers and return values of overriden functions
   */
  void handleMergeOverrides();

  /**
   * Handles the mapping of a call to the called function
   * @param CalledObj
   * @param CE
   */
  void mergeFunctionCall(const StringType &CalledObj, CallInfoConstIterType CE);

  void addCallToCallGraph(const StringType &CallingFunctionName, const StringType &CalledFunctionName);

  void merge(EquivClassesIterator E1, EquivClassesIterator E2);

  EquivClassContainer Data;

  VectorType<std::pair<const clang::FunctionDecl *, const clang::FunctionDecl *>> ParamsToMerge;

  /**
   * Implicit destructor calls that we need to merge. First is the calling function, second the called destructor
   */
  VectorType<std::pair<StringType, StringType>> Destructors;
  void DestructorHelper(ObjectNameDereference ObjDeref, const clang::CXXDestructorDecl *Destructor);
};

// Json stuff

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(FunctionInfo, MangledNames, Parameters, ReferencedInReturnStmts, IsVariadic)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CallInfo, CalledObjects, Arguments)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Prefix, Object, Member)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(EquivClass, Objects, Prefixes)

inline void to_json(nlohmann::json &J, const EquivClassContainer &C) {
  J["EquivClasses"] = C.EquivClasses;
  J["FunctionMap"] = C.FunctionMap;
  J["FunctionInfoMap"] = C.FunctionInfoMap;
  J["CallInfoMap"] = C.CallInfoMap;
  J["CallExprParentMap"] = C.CallExprParentMap;
}
inline void from_json(const nlohmann::json &J, EquivClassContainer &C) {
  J.at("EquivClasses").get_to(C.EquivClasses);
  J.at("FunctionMap").get_to(C.FunctionMap);
  J.at("FunctionInfoMap").get_to(C.FunctionInfoMap);
  J.at("CallInfoMap").get_to(C.CallInfoMap);
  J.at("CallExprParentMap").get_to(C.CallExprParentMap);

  for (auto It = C.EquivClasses.begin(); It != C.EquivClasses.end(); ++It) {
    for (const auto &Obj : It->Objects) {
      C.FindMap.emplace(Obj, It);
    }
  }

  C.InitReferencedInCalls();
}

}  // namespace implementation

// std::hash specializations
template <typename T1, typename T2>
struct std::hash<std::pair<T1, T2>> {
  std::size_t operator()(std::pair<T1, T2> const &O) const noexcept {
    const auto H1 = std::hash<T1>{}(O.first);
    const auto H2 = std::hash<T2>{}(O.second);
    return H1 ^ (H2 + 0x9e3779b9 + (H1 << 6) + (H1 > 2));  // method used by boost::hash_combine
  }
};

template <>
struct std::hash<implementation::CallInfoConstIterType> {
  std::size_t operator()(implementation::CallInfoConstIterType O) const noexcept {
    // Just use the address of the pointed to value, which is stable during a single invocation
    return reinterpret_cast<std::size_t>(&(*(O)));
  }
};

#endif  // CGCOLLECTOR_ALIASANALYSIS_H
