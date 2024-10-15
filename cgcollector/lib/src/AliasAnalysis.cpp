#include "AliasAnalysis.h"
#include "AAReferenceVisitor.h"

#include "AAUSR.h"
#include "helper/common.h"

#include <clang/Index/USRGeneration.h>

#include <unordered_set>
#include <utility>

namespace implementation {

bool ASTInformationExtractor::VisitFunctionDecl(clang::FunctionDecl* FD) {
  const auto Definition = FD->getDefinition();
  if (Definition && Definition != FD) {
    // We are going to visit the definition later anyway, but we need to merge the arguments
    ParamsToMerge.emplace_back(FD, Definition);
    return true;
  }

  auto Obj = std::make_shared<ObjectName>(FD);
  Objects.emplace(Obj);
  auto Return = std::make_shared<ObjectName>(generateUSRForSymbolicReturn(FD));
  Objects.emplace(Return);

  clang::CXXMethodDecl* LambdaCallOperator = nullptr;
  if (auto CXXMethod = llvm::dyn_cast<clang::CXXMethodDecl>(FD)) {
    if (!CXXMethod->isStatic()) {
      // Add the 'this' pointer
      auto ThisObj = std::make_shared<ObjectName>(generateUSRForThisExpr(FD));
      Objects.emplace(ThisObj);
    }
    if (CXXMethod->isLambdaStaticInvoker()) {
      auto Parent = CXXMethod->getParent();
      LambdaCallOperator = Parent->getLambdaCallOperator();
      // The static invoker does not have a body, instead it used the body of the call operator
      // We resolve this by merging the arguments. Return values are handled by the function info class
      ParamsToMerge.emplace_back(CXXMethod, LambdaCallOperator);
    }
    for (auto OverriddenMethod : CXXMethod->overridden_methods()) {
      // We need to do the following:
      // Merge parameters
      ParamsToMerge.emplace_back(CXXMethod, OverriddenMethod);
      // Merge 'this' pointers
      // This cast should be safe, as we do not write to the pointer anywhere. The only reason that we use not const
      // pointers here anyway is limitation that the recursive ast visitor only works with not const pointers
      assert(llvm::isa<clang::CXXMethodDecl>(OverriddenMethod));
      OverridesToMerge.emplace_back(generateUSRForThisExpr(CXXMethod),
                                    generateUSRForThisExpr(const_cast<clang::CXXMethodDecl*>(OverriddenMethod)));
      // Merge return value
      OverridesToMerge.emplace_back(generateUSRForSymbolicReturn(CXXMethod),
                                    generateUSRForSymbolicReturn(const_cast<clang::CXXMethodDecl*>(OverriddenMethod)));
    }
  }

  FuncDeclMap[Obj->Object] = FD;
  Data.FunctionInfoMap.try_emplace(Obj->Object, FD, LambdaCallOperator);
  return true;
}

bool ASTInformationExtractor::VisitVarDecl(clang::VarDecl* VD) {
  // Skip templates
  if (VD->getDeclContext()->isDependentContext()) {
    return true;
  }

  auto Obj = std::make_shared<ObjectName>(VD);
  Objects.emplace(Obj);
  if (VD->hasInit()) {
    const auto ConstructExpr = llvm::dyn_cast<clang::CXXConstructExpr>(VD->getInit());
    if (ConstructExpr) {
      if (CaptureStackCtorsDtors) {
        if (CurrentFunctionDecl) {
          // TODO Constructors outside of main
          // Constructors are pretty similar to method calls, but different in the AST. This sadly leads to some code
          // duplication
          const auto RHS = getReferencedDeclsStr(ConstructExpr, CTX, CurrentFunctionDecl);
          const auto LHS = Obj->getStringRepr();
          ConstructorMergeList.push_back({LHS, RHS});
        }
      }
    } else {
      ObjectNameDereference Deref;
      Deref.setOb(std::make_shared<ObjectName>(VD));
      Initalizers.emplace_back(Deref, VD->getInit(), CurrentFunctionDecl);
    }
  }
  if (const auto RecordType =
          llvm::dyn_cast<clang::RecordType>(stripArrayType(VD->getType()->getUnqualifiedDesugaredType()))) {
    ObjectNameDereference Deref(Obj);
    for (const auto Field : RecordType->getDecl()->fields()) {
      auto ObjMember = std::make_shared<ObjectNameMember>(Deref, generateUSRForDecl(Field->getCanonicalDecl()));
      Objects.emplace(ObjMember);
    }
    if (CaptureStackCtorsDtors) {
      // Parameters are destructed by the caller
      if (!llvm::isa<clang::ParmVarDecl>(VD)) {
        if (const auto CXXClass = llvm::dyn_cast<clang::CXXRecordDecl>(RecordType->getDecl())) {
          const auto Destructor = CXXClass->getDestructor();
          if (Destructor) {
            const auto Tmp = std::make_shared<ObjectName>(VD);
            ObjectNameDereference TmpDeref(Tmp, -1);
            DestructorHelper(TmpDeref, Destructor);
          }
        }
      }
    }
  }
  return true;
}

void ASTInformationExtractor::DestructorHelper(ObjectNameDereference ObjDeref,
                                               const clang::CXXDestructorDecl* Destructor) {
  if (CurrentFunctionDecl) {
    // TODO: We need to be able to handle class outside of main too
    const auto FunctionUSR = generateUSRForDecl(CurrentFunctionDecl);
    const auto DestructorUSR = generateUSRForDecl(Destructor);
    Destructors.emplace_back(FunctionUSR, DestructorUSR);
    Objects.emplace(ObjDeref);
    ThisPointersToMerge.emplace_back(ObjDeref.GetStringRepr(), DestructorUSR);
  }
}

bool ASTInformationExtractor::VisitCompoundAssignOperator(clang::CompoundAssignOperator* CAO) {
  Assignments.emplace_back(CAO, CurrentFunctionDecl);
  return RecursiveASTVisitor::VisitCompoundAssignOperator(CAO);
}

bool ASTInformationExtractor::VisitCallExpr(clang::CallExpr* CE) {
  if (!CurrentFunctionDecl) {
    // TODO: Calls outside of a function, for example in static variable initialization
    return true;
  }
  const auto CallUSr = generateUSRForCallOrConstructExpr(CE, CTX, CurrentFunctionDecl);
  auto OperatorCallExpr = llvm::dyn_cast<clang::CXXOperatorCallExpr>(CE);
  clang::FunctionDecl* OperatorCallCalledFunction = nullptr;
  if (OperatorCallExpr) {
    OperatorCallCalledFunction = getCalledFunctionFromCXXOperatorCallExpr(OperatorCallExpr);
  }
  const auto* OperatorCallCalledMember = llvm::dyn_cast_or_null<clang::CXXMethodDecl>(OperatorCallCalledFunction);
  const CallInfo CI(CE, CTX, CurrentFunctionDecl, OperatorCallCalledMember);

  if (const auto& DC = CE->getDirectCallee()) {
    const auto USR = generateUSRForDecl(DC);

    // TODO: This should always be 1, if it is not its an implementation issue. (for example with typeid)
    if (CI.CalledObjects.size() == 1) {
      DirectCalls.emplace_back(CI, CurrentFunctionDecl, CallUSr, USR);
    }

  } else {
    [[maybe_unused]] const auto Iter = Data.CallInfoMap.insert({CallUSr, CI});
    if (!Iter.second) {
      return true;
    }
    Data.CallExprParentMap[CallUSr] = generateUSRForDecl(CurrentFunctionDecl);
  }
  auto Obj = std::make_shared<ObjectName>(CE, CTX, CurrentFunctionDecl);
  Objects.emplace(Obj);
  if (DereferenceLevel != 0) {
    Objects.emplace(Obj, DereferenceLevel);
  }
  if (auto MemberCallExpr = llvm::dyn_cast<clang::CXXMemberCallExpr>(CE)) {
    auto ImplicitObject = getImplicitObjectFromCXXMemberCall(MemberCallExpr);
    assert(ImplicitObject.first);
    auto Refs = getReferencedDecls(ImplicitObject.first, CTX, CurrentFunctionDecl);
    const auto CalledStmt = getCalledStmtFromCXXMemberCall(MemberCallExpr);
    assert(CalledStmt);
    const auto CalledRefs = getReferencedDecls(CalledStmt, CTX, CurrentFunctionDecl);
    for (auto& Ref : Refs) {
      if (!ImplicitObject.second) {
        // If the 'this' pointer is not already dereferenced we need to do it
        Ref.DereferenceLevel--;
      }
      Objects.emplace(Ref);
      for (const auto& CallRef : CalledRefs) {
        Objects.emplace(CallRef);
        ThisPointersToMerge.emplace_back(Ref.GetStringRepr(), CallRef.GetStringRepr());
      }
    }
  } else if (OperatorCallCalledMember) {
    // If an operatorcall calls a member function we need to merge the 'this' pointer.
    const auto MemberUsr = generateUSRForDecl(OperatorCallCalledMember);
    auto ImplicitObjects = getReferencedDecls(OperatorCallExpr->getArg(0), CTX, CurrentFunctionDecl);
    for (auto& ImplicitObject : ImplicitObjects) {
      ImplicitObject.DereferenceLevel--;
      Objects.emplace(ImplicitObject);
      ThisPointersToMerge.emplace_back(ImplicitObject.GetStringRepr(), MemberUsr);
    }
  }
  return true;
}

bool ASTInformationExtractor::VisitMemberExpr(clang::MemberExpr* ME) {
  const auto Referenced = getReferencedDecls(ME->getBase(), CTX, CurrentFunctionDecl);
  auto MemberDecl = ME->getMemberDecl()->getCanonicalDecl();
  const auto MemberUSR = generateUSRForDecl(MemberDecl);
  const bool IsCXXMember = llvm::isa<clang::CXXMethodDecl>(MemberDecl) || llvm::isa<clang::FieldDecl>(MemberDecl);

  for (const auto& R : Referenced) {
    auto MR = std::make_shared<ObjectNameMember>();
    MR->Member = MemberUSR;
    MR->DB = R;
    if (ME->isArrow()) {
      MR->DB.DereferenceLevel += 1;
    }
    // We need to be careful with the dereference level here. (For example in &a.foo(), a member is accessed which looks
    // like a address of the member, but the address of is applied tp the result of the function call and not to the
    // member itself.)
    ObjectNameDereference DR(MR, DereferenceLevel);
    Objects.insert(DR);
    // If this MemberExpr refers to a C++ Class method, we merge it with the function declaration/definition in the
    // class
    if (IsCXXMember) {
      CXXMemberMergeList.emplace_back(DR.GetStringRepr(), MemberUSR);
    }
    if (ME->isArrow()) {
      Objects.emplace(MR->DB);
    }
  }

  return true;
}

void ASTInformationExtractor::dump() {
  llvm::errs() << "Collected Objects:\n";
  for (const auto& O : Objects) {
    llvm::errs() << O.GetStringRepr() << "\n";
  }
  llvm::errs() << "Equiv Classes:\n";
  for (auto& EquivClasse : Data.EquivClasses) {
    llvm::errs() << "Class: {";
    for (const auto& O : EquivClasse.Objects) {
      llvm::errs() << O << ", ";
    }
    llvm::errs() << "}\n Prefixes: {";
    for (const auto& P : EquivClasse.Prefixes) {
      llvm::errs() << P.GetStringRepr() << ", ";
    }
    llvm::errs() << "}\n";
  }
}

bool ASTInformationExtractor::VisitDeclRefExpr(clang::DeclRefExpr* DE) {
  auto Obj = std::make_shared<ObjectName>(DE->getDecl());
  Objects.emplace(Obj);
  if (DereferenceLevel != 0) {
    Objects.emplace(Obj, DereferenceLevel);
  }
  return true;
}

bool ASTInformationExtractor::VisitCXXThisExpr(clang::CXXThisExpr* TE) {
  auto Obj = std::make_shared<ObjectName>(TE, CurrentFunctionDecl);
  Objects.emplace(Obj);
  if (DereferenceLevel != 0) {
    Objects.emplace(Obj, DereferenceLevel);
  }
  return true;
}

bool ASTInformationExtractor::VisitCXXNewExpr(clang::CXXNewExpr* NE) {
  auto Obj = std::make_shared<ObjectName>(NE, CurrentFunctionDecl);
  // A new expression needs to be able to both represent the allocated object and the returned pointer
  Objects.emplace(Obj);
  Objects.emplace(Obj, DereferenceLevel - 1);
  if (DereferenceLevel != 0) {
    Objects.emplace(Obj, DereferenceLevel);
    Objects.emplace(Obj, DereferenceLevel - 1);
  }
  // And it can have an init too
  if (NE->hasInitializer()) {
    const auto ConstructExpr = llvm::dyn_cast<clang::CXXConstructExpr>(NE->getInitializer());
    if (ConstructExpr) {
      if (CurrentFunctionDecl && CaptureCtorsDtors) {
        // TODO Constructors outside of main
        // Constructors are pretty similar to method calls, but different in the AST. This sadly leads to some code
        // duplication
        const auto RHS = getReferencedDeclsStr(NE->getInitializer(), CTX, CurrentFunctionDecl);
        // TODO: We reuse the constructor list here, maybe use a separate one
        ConstructorMergeList.push_back({Obj->getStringRepr(), RHS});
      }
    } else {
      ObjectNameDereference Deref;
      Deref.setOb(std::make_shared<ObjectName>(NE, CurrentFunctionDecl));
      // We assume that
      // NE->getInitializer()->getType()->getUnqualifiedDesugaredType() ==
      //      NE->getAllocatedType()->getUnqualifiedDesugaredType());
      // always holds
      Initalizers.emplace_back(Deref, NE->getInitializer(), CurrentFunctionDecl);
    }
  }

  // FIXME : This is a hacky solution to get placment new to work. The real one would be to potentially call the
  // operator new
  if (NE->getNumPlacementArgs() == 1) {
    // We assume this is a placement new.
    const auto RHS = getReferencedDeclsStr(NE->getPlacementArg(0), CTX, CurrentFunctionDecl);
    // TODO: We reuse the constructor list here, maybe use a separate one
    ConstructorMergeList.push_back({Obj->getStringRepr(), RHS});
  }
  return true;
}

#if LLVM_VERSION_MAJOR < 11
bool ASTInformationExtractor::TraverseUnaryDeref(clang::UnaryOperator* UO, [[maybe_unused]] DataRecursionQueue* Queue) {
  if (!WalkUpFromUnaryDeref(UO)) {
    return false;
  }
  DereferenceLevel += 1;
  // No queue to force an immediate visit
  const bool RetValue = TraverseStmt(UO->getSubExpr());
  DereferenceLevel -= 1;
  return RetValue;
}

bool ASTInformationExtractor::TraverseUnaryAddrOf(clang::UnaryOperator* UO,
                                                  [[maybe_unused]] DataRecursionQueue* Queue) {
  if (!WalkUpFromUnaryAddrOf(UO)) {
    return false;
  }
  DereferenceLevel -= 1;
  // No queue here to force an immediate visit
  const bool RetValue = TraverseStmt(UO->getSubExpr());
  DereferenceLevel += 1;
  return RetValue;
}
#else
bool ASTInformationExtractor::TraverseUnaryOperator(clang::UnaryOperator* UO,
                                                    [[maybe_unused]] DataRecursionQueue* Queue) {
  const auto Opcode = UO->getOpcode();
  if (Opcode == clang::UnaryOperatorKind::UO_AddrOf || Opcode == clang::UnaryOperatorKind::UO_Deref) {
    if (!WalkUpFromUnaryOperator(UO)) {
      return false;
    }
    if (Opcode == clang::UnaryOperatorKind::UO_AddrOf) {
      DereferenceLevel -= 1;
    } else /* UO_Deref */ {
      DereferenceLevel += 1;
    }
    // No queue here to force an immediate visit
    const bool RetValue = TraverseStmt(UO->getSubExpr());
    if (Opcode == clang::UnaryOperatorKind::UO_AddrOf) {
      DereferenceLevel += 1;
    } else /* UO_Deref */ {
      DereferenceLevel -= 1;
    }
    return RetValue;
  }
  return RecursiveASTVisitor::TraverseUnaryOperator(UO, Queue);
}
#endif

bool ASTInformationExtractor::TraverseArraySubscriptExpr(clang::ArraySubscriptExpr* Expr,
                                                         [[maybe_unused]] DataRecursionQueue* Queue) {
  if (!WalkUpFromArraySubscriptExpr(Expr)) {
    return false;
  }

  bool Ret = TraverseStmt(Expr->getBase());
  const auto Prev = DereferenceLevel;
  DereferenceLevel = 0;
  // No queue here to force an immediate visit
  Ret &= TraverseStmt(Expr->getIdx());
  DereferenceLevel = Prev;
  return Ret;
}

bool ASTInformationExtractor::TraverseBinaryOperator(clang::BinaryOperator* BO,
                                                     [[maybe_unused]] DataRecursionQueue* Queue) {
  if (!WalkUpFromBinaryOperator(BO)) {
    return false;
  }

  bool Ret = TraverseStmt(BO->getLHS());
  const auto Prev = DereferenceLevel;
  DereferenceLevel = 0;
  // No queue here to force an immediate visit
  Ret &= TraverseStmt(BO->getRHS());
  DereferenceLevel = Prev;
  return Ret;
}

#if LLVM_VERSION_MAJOR < 11
bool ASTInformationExtractor::VisitBinAssign(clang::BinaryOperator* BO) {
  Assignments.emplace_back(BO, CurrentFunctionDecl);
  return true;
}
#else
bool ASTInformationExtractor::VisitBinaryOperator(clang::BinaryOperator* BO) {
  if (BO->getOpcode() == clang::BinaryOperatorKind::BO_Assign) {
    Assignments.emplace_back(BO, CurrentFunctionDecl);
    return true;
  }
  return RecursiveASTVisitor::VisitBinaryOperator(BO);
}
#endif

void ASTInformationExtractor::initEquivClasses() {
  for (const auto& Object : Objects) {
    const auto StringRepr = Object.GetStringRepr();
    Data.EquivClasses.emplace_front(StringRepr);
    Data.FindMap.emplace(StringRepr, Data.EquivClasses.begin());
    if (Object.GetFunctionName()) {
      Data.FunctionMap.emplace(StringRepr, Object.GetFunctionName().value());
    }
  }
}

void ASTInformationExtractor::initPrefixClasses() {
  for (const auto& Object : Objects) {
    // Line 5 - 6
    auto Tmp = Object;
    Tmp.DereferenceLevel += 1;
    auto TmpObject = Objects.find(Tmp);
    if (TmpObject != Objects.end()) {
      auto& Pre = Data.FindMap[Object.GetStringRepr()]->Prefixes;
      Pre.emplace_back((*TmpObject));
    } else {
      // Line 7 - 8 These lines are in the original algorithm description, but I don't really see how they differ from
      // the previous two. In practice this code leads to duplications in the prefix classes
      //      Tmp.DereferenceLevel = Object.DereferenceLevel - 1;
      //      TmpObject = Objects.find(Tmp);
      //      if (TmpObject != Objects.end()) {
      //        auto &Pre = Data.FindMap[TmpObject->GetStringRepr()]->Prefixes;
      //        Pre.emplace_back(Object);
      //      }
    }
    // Line 9 - 10
    if (Object.DereferenceLevel == 0 && Object.getMb()) {
      const auto Tmp = Object.getMb()->DB;
      const auto TmpObject = Objects.find(Tmp);
      if (TmpObject != Objects.end()) {
        auto& Pre = Data.FindMap[TmpObject->GetStringRepr()]->Prefixes;
        Pre.emplace_back(Object, Object.getMb()->Member);
      }
    }
  }
}

void ASTInformationExtractor::calculatePERelation() {
  initEquivClasses();
  initPrefixClasses();
  Data.InitReferencedInCalls();

  handleDirectCalls();
  handleDestructorCalls();
  handleMergeCXXMembers();
  handleMergeFunctionParameters();
  handleMergeOverrides();
  handleInits();
  handleAssignments();
  handleMergeConstructorAssignments();
  handleMergeCXXThisPointers();
  // dump();
}

void ASTInformationExtractor::merge(EquivClassesIterator E1, EquivClassesIterator E2) {
  const auto Ret = ::implementation::mergeRecurisve(Data, E1, E2);
  for (const auto& [Obj, CE] : Ret.first) {
    mergeFunctionCall(Obj, CE);
  }
}

void ASTInformationExtractor::handleAssignments() {
  for (const auto& Assignment : Assignments) {
    const auto LHS = getReferencedDeclsStr(Assignment.first->getLHS(), CTX, Assignment.second);
    const auto RHS = getReferencedDeclsStr(Assignment.first->getRHS(), CTX, Assignment.second);
    assert(!LHS.empty());
    for (const auto& L : LHS) {
      const auto ItL = Data.FindMap.find(L);
      assert(ItL != Data.FindMap.end());
      for (const auto& R : RHS) {
        const auto ItR = Data.FindMap.find(R);
        assert(ItR != Data.FindMap.end());
        if (ItL->second != ItR->second) {
          merge(ItL->second, ItR->second);
        }
      }
    }
  }
}

void ASTInformationExtractor::handleInits() {
  for (const auto& Decl : Initalizers) {
    if (const auto ConstructExpr = llvm::dyn_cast<clang::CXXConstructExpr>(Decl.Init)) {
      llvm_unreachable("Construct expressions need to be handled before");
    } else {
      const auto InitExpr = Decl.Init;
      if (const auto InitListExpr = llvm::dyn_cast<clang::InitListExpr>(InitExpr)) {
        const auto SemanticInitExpr = (InitListExpr->isSemanticForm() ? InitListExpr : InitListExpr->getSemanticForm());
        handleInitExpr(&Decl.Object, SemanticInitExpr, SemanticInitExpr->getType()->getUnqualifiedDesugaredType(),
                       Decl.ParentFunction);
      } else {
        handleInitExpr(&Decl.Object, InitExpr, InitExpr->getType()->getUnqualifiedDesugaredType(), Decl.ParentFunction);
      }
    }
  }
}

void ASTInformationExtractor::handleInitExpr(const ObjectNameDereference* ObjDeref, clang::Stmt* InitExpr,
                                             const clang::Type* VarType, clang::FunctionDecl* ParentFunctionDecl) {
  if (const auto ArrayType = llvm::dyn_cast<clang::ArrayType>(VarType)) {
    if (const auto InitListExpr = llvm::dyn_cast<clang::InitListExpr>(InitExpr)) {
      const auto TmpType = ArrayType->getElementType()->getUnqualifiedDesugaredType();
      for (const auto InitListChild : InitListExpr->children()) {
        handleInitExpr(ObjDeref, InitListChild, TmpType, ParentFunctionDecl);
      }
    } else if (const auto InitLoopExpr = llvm::dyn_cast<clang::ArrayInitLoopExpr>(InitExpr)) {
      llvm::errs() << "ArrayInitLoopExpr are not yet implemented\n";
    } else {
      // String Literals are a separate type in clang
      if (!llvm::isa<clang::StringLiteral>(InitExpr)) {
        assert(false && "Expected array to have initialization  list");
      }
    }

  } else if (const auto StructType = llvm::dyn_cast<clang::RecordType>(VarType)) {
    if (StructType->getDecl()->isUnion()) {
      // TODO Initialisation of unions
      llvm::errs() << "Init Expressions for unions not yet supported\n";
#ifdef DEBUG_TEST_AA
      InitExpr->dump();
      InitExpr->getBeginLoc().dump(CTX->getSourceManager());
#endif
    } else {
      // Struct
      if (const auto InitListExpr = llvm::dyn_cast<clang::InitListExpr>(InitExpr)) {
        // Init with init list
        auto InitIter = InitListExpr->begin();
        const auto StructDecl = StructType->getDecl();
        for (const auto Field : StructDecl->fields()) {
          assert(InitIter != InitListExpr->end());
          auto ObjMember = std::make_shared<ObjectNameMember>(*ObjDeref, generateUSRForDecl(Field->getCanonicalDecl()));
          const ObjectNameDereference Tmp(ObjMember);
          handleInitExpr(&Tmp, *InitIter, Field->getType()->getUnqualifiedDesugaredType(), ParentFunctionDecl);
          InitIter++;
        }

      } else {
        const auto Referenced = getReferencedDeclsStr(InitExpr, CTX, ParentFunctionDecl);
        const auto LHS = Data.FindMap.find(ObjDeref->GetStringRepr());
        assert(LHS != Data.FindMap.end());
        for (const auto& R : Referenced) {
          const auto RHS = Data.FindMap.find(R);
          assert(RHS != Data.FindMap.end());
          if (LHS->second != RHS->second) {
            merge(LHS->second, RHS->second);
          }
        }
      }
    }
  } else {
    // Simple types
    const auto Refs = getReferencedDeclsStr(InitExpr, CTX, ParentFunctionDecl);
    // Ref can be empty, for example if a variable is initialized with a literal expression
    const auto ItObj = Data.FindMap.find(ObjDeref->GetStringRepr());
    assert(ItObj != Data.FindMap.end());
    for (const auto& Ref : Refs) {
      const auto ItRef = Data.FindMap.find(Ref);
      assert(ItRef != Data.FindMap.end());
      if (ItObj->second != ItRef->second) {
        merge(ItObj->second, ItRef->second);
      }
    }
  }
}

const clang::Type* ASTInformationExtractor::stripArrayType(const clang::Type* Type) const {
  Type = Type->getUnqualifiedDesugaredType();
  while (const auto AType = llvm::dyn_cast<clang::ArrayType>(Type)) {
    Type = AType->getElementType()->getUnqualifiedDesugaredType();
  }
  return Type;
}

void ASTInformationExtractor::mergeFunctionCall(const StringType& CalledObj, CallInfoConstIterType CE) {
  static std::unordered_set<std::pair<StringType, CallInfoConstIterType>> Cache;
  if (!Cache.emplace(std::make_pair(CalledObj, CE)).second) {
    return;
  }
  auto Ret = ::implementation::mergeFunctionCall(Data, CalledObj, CE);
  if (Ret) {
    addCallToCallGraph(Ret->second.first, Ret->second.second);
    for (const auto& ToMerge : Ret->first) {
      mergeFunctionCall(ToMerge.first, ToMerge.second);
    }
  }
}

ASTInformationExtractor::ASTInformationExtractor(const clang::ASTContext* CTX, CallGraph* CG, bool CaptureCtorsDtors,
                                                 bool CaptureStackCtorsDtors)
    : CTX(CTX), CG(CG), CaptureCtorsDtors(CaptureCtorsDtors), CaptureStackCtorsDtors(CaptureStackCtorsDtors) {}

const EquivClassContainer& ASTInformationExtractor::getData() const { return Data; }

void ASTInformationExtractor::addCallToCallGraph(const StringType& CallingFunctionName,
                                                 const StringType& CalledFunctionName) {
  auto CallingFunction = FuncDeclMap.find(CallingFunctionName);
  assert(CallingFunction != FuncDeclMap.end());
  auto CalledFunction = FuncDeclMap.find(CalledFunctionName);
  if (CalledFunction != FuncDeclMap.end()) {
    auto Called = CG->getOrInsertNode(CalledFunction->second);
    CG->getOrInsertNode(CallingFunction->second)->addCallee(Called);
  } else {
    assert(false);
    llvm::errs() << "Warning: We found a call originating in the function " << CallingFunctionName
                 << " to the function " << CalledFunctionName << " but the called function does not exist\n";
  }
}

bool ASTInformationExtractor::TraverseTypedefDecl(clang::TypedefDecl* TDD) {
  // This is needed to reach the decls of builtin structs
  if (TDD->isImplicit()) {
    const bool Prev = TraverseTypeLocs;
    TraverseTypeLocs = true;
    TraverseTypesOfTypeLocs = true;
    InTypededDecl = true;
    const bool Ret = RecursiveASTVisitor::TraverseTypedefDecl(TDD);
    InTypededDecl = false;
    TraverseTypesOfTypeLocs = false;
    TraverseTypeLocs = Prev;
    return Ret;
  }
  return true;
}

bool ASTInformationExtractor::TraverseTypeAliasDecl(clang::TypeAliasDecl*) { return true; };

void ASTInformationExtractor::handleMergeFunctionParameters() {
  for (const auto& [F1, F2] : ParamsToMerge) {
    const auto& F1Params = F1->parameters();
    const auto& F2Params = F2->parameters();
    const unsigned int NumParams = F1->getNumParams();
    for (unsigned i = 0; i < NumParams; i++) {
      const auto Decls1 = getDecls(F1Params[i], &F1->getASTContext());
      assert(Decls1.size() == 1);
      const auto Decls2 = getDecls(F2Params[i], &F2->getASTContext());
      assert(Decls2.size() == 1);
      const auto Iter1 = Data.FindMap.find(Decls1[0]);
      assert(Iter1 != Data.FindMap.end());
      const auto Iter2 = Data.FindMap.find(Decls2[0]);
      assert(Iter2 != Data.FindMap.end());
      if (Iter1->second != Iter2->second) {
        merge(Iter1->second, Iter2->second);
      }
    }
  }
}

bool ASTInformationExtractor::TraverseParmVarDecl(clang::ParmVarDecl* PD) {
  const bool PreviousShouldVisitType = TraverseTypeLocs;
  TraverseTypeLocs = false;
  const auto Ret = RecursiveASTVisitor::TraverseParmVarDecl(PD);
  TraverseTypeLocs = PreviousShouldVisitType;
  return Ret;
}

bool ASTInformationExtractor::TraverseTypeLoc(clang::TypeLoc TL) {
  if (!TraverseTypeLocs) {
    return true;
  }
  return RecursiveASTVisitor::TraverseTypeLoc(TL);
}

bool ASTInformationExtractor::TraverseCXXMethodDecl(clang::CXXMethodDecl* MD) {
  return TraverseFunctionLikeHelper(MD, &RecursiveASTVisitor::TraverseCXXMethodDecl);
}

bool ASTInformationExtractor::TraverseFunctionDecl(clang::FunctionDecl* FD) {
  return TraverseFunctionLikeHelper(FD, &RecursiveASTVisitor::TraverseFunctionDecl);
}

bool ASTInformationExtractor::TraverseCXXDeductionGuideDecl(clang::CXXDeductionGuideDecl* DD) {
  return TraverseFunctionLikeHelper(DD, &RecursiveASTVisitor::TraverseCXXDeductionGuideDecl);
}

bool ASTInformationExtractor::TraverseCXXConstructorDecl(clang::CXXConstructorDecl* CD) {
  return TraverseFunctionLikeHelper(CD, &RecursiveASTVisitor::TraverseCXXConstructorDecl);
}

bool ASTInformationExtractor::TraverseCXXConversionDecl(clang::CXXConversionDecl* CD) {
  return TraverseFunctionLikeHelper(CD, &RecursiveASTVisitor::TraverseCXXConversionDecl);
}

bool ASTInformationExtractor::TraverseCXXDestructorDecl(clang::CXXDestructorDecl* DD) {
  return TraverseFunctionLikeHelper(DD, &RecursiveASTVisitor::TraverseCXXDestructorDecl);
}

void ASTInformationExtractor::handleMergeCXXMembers() {
  for (const auto& MethodsToMerge : CXXMemberMergeList) {
    const auto It1 = Data.FindMap.find(MethodsToMerge.first);
    assert(It1 != Data.FindMap.end());
    const auto It2 = Data.FindMap.find(MethodsToMerge.second);
    assert(It2 != Data.FindMap.end());
    if (It1->second != It2->second) {
      merge(It1->second, It2->second);
    }
  }
}

void ASTInformationExtractor::handleMergeCXXThisPointers() {
  for (const auto& ToMerge : ThisPointersToMerge) {
    // We need to check all methods that share an EquivalenceClass with the called method
    const auto CalledEquivClassIter = Data.FindMap.find(ToMerge.second);
    assert(CalledEquivClassIter != Data.FindMap.end());
    for (const auto& PotentialMethod : CalledEquivClassIter->second->Objects) {
      const auto FoundMethodIter = Data.FunctionMap.find(PotentialMethod);
      if (FoundMethodIter != Data.FunctionMap.end()) {
        const auto ActuallyCalledMethod = FoundMethodIter->second;
        // We found the called Method. Now we need to merge its 'this' pointer with the implicit object
        const auto ThisUSR = generateUSRForThisExpr(ActuallyCalledMethod);
        const auto ThisIter = Data.FindMap.find(ThisUSR);
        // It could happen that a method without 'this' pointer is for some reason in the same equivalence class
        if (ThisIter != Data.FindMap.end()) {
          const auto CallingObjectIter = Data.FindMap.find(ToMerge.first);
          assert(CallingObjectIter != Data.FindMap.end());
          if (ThisIter->second != CallingObjectIter->second) {
            merge(ThisIter->second, CallingObjectIter->second);
          }
        }
      }
    }
  }
}

bool ASTInformationExtractor::TraverseCompoundStmt(clang::CompoundStmt* CS) {
  const auto PrevTraverseTypeLocs = TraverseTypeLocs;
  TraverseTypeLocs = false;
  const auto Ret = RecursiveASTVisitor::TraverseCompoundStmt(CS);
  TraverseTypeLocs = PrevTraverseTypeLocs;
  return Ret;
}

void ASTInformationExtractor::handleDestructorCalls() {
  for (const auto& Destructor : Destructors) {
    addCallToCallGraph(Destructor.first, Destructor.second);
  }
}

void ASTInformationExtractor::ConstructExprHelper(clang::CXXConstructExpr* CE, const ObjectNameDereference& ThisDeref) {
  Objects.emplace(ThisDeref);
  ConstructExprHelper(CE, ThisDeref.GetStringRepr());
}

void ASTInformationExtractor::ConstructExprHelper(clang::CXXConstructExpr* CE, const StringType& ThisDeref) {
  // Constructor calls are handled similar to direct calls. The main differences are that they have a 'this' pointer and
  // that constructors can not have a return value. The third field in the 'DirectCalls' list is used to assign the
  // return value of a function to the result of calling it, but because constructors can not have a return value we
  // instead pass just 'none'. (This field is not read later anyway if the called function does not return anything)
  assert(CurrentFunctionDecl);
  const CallInfo CI(CE, CTX, CurrentFunctionDecl);
  const auto CalledUSR = generateUSRForDecl(CE->getConstructor());
  DirectCalls.emplace_back(CI, CurrentFunctionDecl, "none", CalledUSR);

  ThisPointersToMerge.emplace_back(ThisDeref, generateUSRForDecl(CE->getConstructor()));
}

bool ASTInformationExtractor::VisitCXXDeleteExpr(clang::CXXDeleteExpr* DE) {
  if (!CaptureCtorsDtors) {
    return true;
  }
  if (const auto RecordType =
          llvm::dyn_cast<clang::RecordType>(stripArrayType(DE->getDestroyedType()->getUnqualifiedDesugaredType()))) {
    if (const auto CXXClass = llvm::dyn_cast<clang::CXXRecordDecl>(RecordType->getDecl())) {
      const auto Destructor = CXXClass->getDestructor();
      if (Destructor) {
        auto Referenced = getReferencedDecls(DE->getArgument(), CTX, CurrentFunctionDecl);
        for (auto& Ref : Referenced) {
          Ref.DereferenceLevel--;
          DestructorHelper(Ref, Destructor);
        }
      }
    }
  }
  return true;
}

// There is no VisitConstructorInitializer function, so we need to do everything we would have done in the visit method
// here, but still call the original travers
bool ASTInformationExtractor::TraverseConstructorInitializer(clang::CXXCtorInitializer* CtorInit) {
  if (CtorInit->isAnyMemberInitializer()) {
    // TODO Code duplication with var init
    const auto ConstructExpr = llvm::dyn_cast<clang::CXXConstructExpr>(CtorInit->getInit());
    if (ConstructExpr) {
      if (CaptureStackCtorsDtors) {
        if (CurrentFunctionDecl) {
          // TODO Constructors outside of main
          // Constructors are pretty similar to method calls, but different in the AST. This sadly leads to some code
          // duplication
          const auto ReferencedUSR = generateUSRForDecl(CtorInit->getAnyMember());
          const auto RHS = getReferencedDeclsStr(ConstructExpr, CTX, CurrentFunctionDecl);
          ConstructorMergeList.push_back({ReferencedUSR, RHS});
        }
      }
    } else {
      const auto ReferencedUSR = generateUSRForDecl(CtorInit->getAnyMember());
      const ObjectNameDereference Deref(std::make_shared<ObjectName>(ReferencedUSR));
      Initalizers.emplace_back(Deref, CtorInit->getInit(), CurrentFunctionDecl);
    }
  } else {
    // Base initializer
    if (CaptureStackCtorsDtors) {
      //  TODO: Base initialisers are not always just CXXConstructExpr, they can also contain ExprWithCleanups and
      //  possible other expressions. See std::system_error for an example
      // auto Init = CtorInit->getInit();
      // if (auto ExprClean = llvm::dyn_cast<clang::ExprWithCleanups>(Init)) {
      //  Init = ExprClean->getSubExpr();
      //}
      const auto ConstructExpr = llvm::dyn_cast<clang::CXXConstructExpr>(CtorInit->getInit());
      if (ConstructExpr) {
        const StringType ThisUSR(generateUSRForThisExpr(CurrentFunctionDecl));
        const StringType ThisUSRRHS(generateUSRForThisExpr(ConstructExpr->getConstructor()));
        VectorType<StringType> RHS;
        RHS.push_back(ThisUSRRHS);
        ConstructorMergeList.push_back({ThisUSR, RHS});
      }
    }
  }

  return RecursiveASTVisitor::TraverseConstructorInitializer(CtorInit);
}

bool ASTInformationExtractor::VisitFieldDecl(clang::FieldDecl* FD) {
  if (FD->isUnnamedBitfield()) {
    // Unnamed bitfields can not be referenced
    return true;
  }
  const auto Obj = std::make_shared<ObjectName>(FD);
  Objects.emplace(Obj);
  return true;
}

void ASTInformationExtractor::handleMergeOverrides() {
  for (const auto& ToMerge : OverridesToMerge) {
    const auto ItFirst = Data.FindMap.find(ToMerge.first);
    assert(ItFirst != Data.FindMap.end());
    const auto ItSecond = Data.FindMap.find(ToMerge.second);
    assert(ItSecond != Data.FindMap.end());
    if (ItFirst->second != ItSecond->second) {
      merge(ItFirst->second, ItSecond->second);
    }
  }
}

void ASTInformationExtractor::handleDirectCalls() {
  for (const auto& Call : DirectCalls) {
    const auto CallInfo = std::get<0>(Call);
    const auto ParentDecl = std::get<1>(Call);
    const auto CallerUSR = generateUSRForDecl(ParentDecl);
    const auto& CallExprUSR = std::get<2>(Call);
    const auto& CalledUSR = std::get<3>(Call);
    assert(CallInfo.CalledObjects.size() == 1);
    auto Ret = ::implementation::mergeFunctionCallImpl(Data, CalledUSR, CallerUSR, CallInfo, CallExprUSR);
    if (Ret) {
      addCallToCallGraph(Ret->second.first, Ret->second.second);
      for (const auto& ToMerge : Ret->first) {
        mergeFunctionCall(ToMerge.first, ToMerge.second);
      }
    }
  }
}

bool ASTInformationExtractor::VisitCXXConstructExpr(clang::CXXConstructExpr* CE) {
  if (InNew && !CaptureCtorsDtors) {
    return true;
  }
  if (CaptureStackCtorsDtors) {
    if (CurrentFunctionDecl) {
      // TODO Constructors outside of main
      // Constructors are pretty similar to method calls, but different in the AST. This sadly leads to some code
      // duplication
      const auto ReferencedUSR = generateUSRForCallOrConstructExpr(CE, CTX, CurrentFunctionDecl);
      const auto Tmp = std::make_shared<ObjectName>(ReferencedUSR);
      const ObjectNameDereference TmpDeref(Tmp, -1);
      Objects.emplace(TmpDeref);
      ConstructExprHelper(CE, TmpDeref);
    }
  }

  // TODO Functions outside of main
  if (CurrentFunctionDecl) {
    auto Obj = std::make_shared<ObjectName>(CE, CTX, CurrentFunctionDecl);
    Objects.emplace(Obj);
    if (DereferenceLevel != 0) {
      Objects.emplace(Obj, DereferenceLevel);
    }
  }

  return true;
}

bool ASTInformationExtractor::VisitMaterializeTemporaryExpr(clang::MaterializeTemporaryExpr* MTE) {
  if (!CurrentFunctionDecl) {
    // TODO: MaterializeTemporaryExpr can be outside of functions
    return true;
  }
  auto Obj = std::make_shared<ObjectName>(MTE, CurrentFunctionDecl);
  Objects.emplace(Obj);
  if (DereferenceLevel != 0) {
    Objects.emplace(Obj, DereferenceLevel);
  }
  const auto LHS = generateUSRForMaterializeTemporaryExpr(MTE, CurrentFunctionDecl);
  const auto RHS = getReferencedDeclsStr(MTE->getSubExpr(), CTX, CurrentFunctionDecl);
  ConstructorMergeList.push_back({LHS, RHS});
  return true;
}

void ASTInformationExtractor::handleMergeConstructorAssignments() {
  for (const auto& Assignment : ConstructorMergeList) {
    const auto& LHS = Assignment.first;
    const auto& RHS = Assignment.second;
    assert(!LHS.empty());
    const auto ItL = Data.FindMap.find(LHS);
    assert(ItL != Data.FindMap.end());
    for (const auto& R : RHS) {
      const auto ItR = Data.FindMap.find(R);
      assert(ItR != Data.FindMap.end());
      if (ItL->second != ItR->second) {
        merge(ItL->second, ItR->second);
      }
    }
  }
}

bool ASTInformationExtractor::VisitCXXBindTemporaryExpr(clang::CXXBindTemporaryExpr* BTE) {
  if (!CaptureCtorsDtors) {
    return true;
  }
  const auto Destructor = BTE->getTemporary()->getDestructor();
  if (Destructor) {
    auto Referenced = getReferencedDecls(BTE->getSubExpr(), CTX, CurrentFunctionDecl);
    for (auto& Ref : Referenced) {
      Ref.DereferenceLevel--;
      DestructorHelper(Ref, Destructor);
    }
  }

  return true;
}

bool ASTInformationExtractor::TraverseCXXNewExpr(clang::CXXNewExpr* NE, [[maybe_unused]] DataRecursionQueue* Queue) {
  InNew = true;
  const bool Ret = RecursiveASTVisitor::TraverseCXXNewExpr(NE, nullptr);
  InNew = false;
  return Ret;
}

bool ASTInformationExtractor::TraverseStaticAssertDecl(clang::StaticAssertDecl*) { return true; }
bool ASTInformationExtractor::TraverseDeclarationNameInfo(clang::DeclarationNameInfo) { return true; }
bool ASTInformationExtractor::TraverseRecordDecl(clang::RecordDecl* RD) {
  if (RD->getDeclContext()->isDependentContext()) {
    return true;
  }
  const auto Ret = RecursiveASTVisitor::TraverseRecordDecl(RD);
  return Ret;
}

bool ASTInformationExtractor::TraverseCXXRecordDecl(clang::CXXRecordDecl* RD) {
  if (RD->getDeclContext()->isDependentContext()) {
    return true;
  }
  const auto Ret = RecursiveASTVisitor::TraverseCXXRecordDecl(RD);
  return Ret;
}
bool ASTInformationExtractor::VisitRecordType(clang::RecordType* RT) {
  // We need to be careful not to run into infinite recursions here
  if ((!InTypededDecl) || (RT == CurrentRecordType)) {
    return true;
  }
  TraverseTypesOfTypeLocs = false;
  TraverseTypeLocs = false;

  CurrentRecordType = RT;
  const bool Ret = TraverseRecordDecl(RT->getDecl());
  CurrentRecordType = nullptr;
  return Ret;
}
bool ASTInformationExtractor::TraverseCXXNoexceptExpr(clang::CXXNoexceptExpr*, DataRecursionQueue*) { return true; }

// This is the expanded code from clangs AST visitor, modified to skip exception specifieres and similar
bool ASTInformationExtractor::TraverseFunctionProtoTypeLoc(clang::FunctionProtoTypeLoc TL) {
  if (!getDerived().shouldTraversePostOrder()) {
    if (!getDerived().WalkUpFromFunctionProtoTypeLoc(TL))
      return false;
    if (getDerived().shouldWalkTypesOfTypeLocs()) {
      if (!getDerived().WalkUpFromFunctionProtoType(const_cast<clang::FunctionProtoType*>(TL.getTypePtr()))) {
        return false;
      }
    }
  }
  if (!getDerived().TraverseTypeLoc(TL.getReturnLoc()))
    return false;
  const clang::FunctionProtoType* T = TL.getTypePtr();
  for (unsigned i = 0, E = TL.getNumParams(); i != E; ++i) {
    if (TL.getParam(i)) {
      if (!getDerived().TraverseDecl(TL.getParam(i))) {
        return false;
      }
    } else if (i < T->getNumParams()) {
      if (!getDerived().TraverseType(T->getParamType(i))) {
        return false;
      }
    }
  }
  if (getDerived().shouldTraversePostOrder()) {
    if (!getDerived().WalkUpFromFunctionProtoTypeLoc(TL)) {
      return false;
    }
    if (getDerived().shouldWalkTypesOfTypeLocs())
      if (!getDerived().WalkUpFromFunctionProtoType(const_cast<clang::FunctionProtoType*>(TL.getTypePtr()))) {
        return false;
      }
  }
  return true;
}

bool ObjectName::operator<(const ObjectName& rhs) const { return Object < rhs.Object; }

bool ObjectName::operator>(const ObjectName& rhs) const { return rhs < *this; }

bool ObjectName::operator<=(const ObjectName& rhs) const { return !(rhs < *this); }

bool ObjectName::operator>=(const ObjectName& rhs) const { return !(*this < rhs); }

bool ObjectName::operator==(const ObjectName& rhs) const { return Object == rhs.Object; }

bool ObjectName::operator!=(const ObjectName& rhs) const { return !(rhs == *this); }

StringType ObjectName::getStringRepr() const { return Object; }

ObjectName::ObjectName(clang::NamedDecl* Decl) {
  Object = generateUSRForDecl(Decl);
  if (llvm::isa<clang::FunctionDecl>(Decl)) {
    Function = true;
  }
}

ObjectName::ObjectName(clang::CallExpr* CE, const clang::ASTContext* CTX, clang::FunctionDecl* ParentFunctionDecl) {
  Object = generateUSRForCallOrConstructExpr(CE, CTX, ParentFunctionDecl);
}

std::optional<StringType> ObjectName::GetFunctionName() const {
  if (Function) {
    return Object;
  }
  return {};
}

ObjectName::ObjectName(StringType Name) : Object(std::move(Name)) {}

ObjectName::ObjectName(const clang::CXXThisExpr*, clang::FunctionDecl* ParentFunctionDecl) {
  Object = generateUSRForThisExpr(ParentFunctionDecl);
}

ObjectName::ObjectName(clang::CXXNewExpr* NE, clang::FunctionDecl* ParentFunctionDecl) {
  Object = generateUSRForNewExpr(NE, ParentFunctionDecl);
}

ObjectName::ObjectName(clang::MaterializeTemporaryExpr* MTE, clang::FunctionDecl* ParentFunctionDecl) {
  Object = generateUSRForMaterializeTemporaryExpr(MTE, ParentFunctionDecl);
}

ObjectName::ObjectName(clang::CXXConstructExpr* CE, const clang::ASTContext* CTX,
                       clang::FunctionDecl* ParentFunctionDecl) {
  Object = generateUSRForCallOrConstructExpr(CE, CTX, ParentFunctionDecl);
}

// TODO: Typeid
// ObjectName::ObjectName(const clang::CXXTypeidExpr *TE) { Object = generateUSRForTypeidExpr(TE); }
// TODO: This check for equality is recursive, is this intended?
bool ObjectNameMember::operator==(const ObjectNameMember& Rhs) const { return DB == Rhs.DB && Member == Rhs.Member; }

bool ObjectNameMember::operator!=(const ObjectNameMember& rhs) const { return !(rhs == *this); }

StringType ObjectNameMember::GetStringRepr() const {
  StringType Output;
  Output += "(";
  Output += DB.GetStringRepr();
  Output += ").";
  Output += Member;
  return Output;
}

bool ObjectNameMember::operator<(const ObjectNameMember& Rhs) const {
  if (DB < Rhs.DB) {
    return true;
  }
  if (Rhs.DB < DB) {
    return false;
  }
  return Member < Rhs.Member;
}

bool ObjectNameMember::operator>(const ObjectNameMember& rhs) const { return rhs < *this; }

bool ObjectNameMember::operator<=(const ObjectNameMember& rhs) const { return !(rhs < *this); }

bool ObjectNameMember::operator>=(const ObjectNameMember& rhs) const { return !(*this < rhs); }

ObjectNameMember::ObjectNameMember(ObjectNameDereference DB, StringType Member)
    : DB(std::move(DB)), Member(std::move(Member)) {}

bool ObjectNameDereference::operator==(const ObjectNameDereference& Rhs) const {
  assert(getOb() || getMb());
  assert(Rhs.getOb() || Rhs.getMb());
  if (getOb() && Rhs.getOb()) {
    return *getOb() == *Rhs.getOb() && DereferenceLevel == Rhs.DereferenceLevel;
  }
  if (getMb() && Rhs.getMb()) {
    return *getMb() == *Rhs.getMb() && DereferenceLevel == Rhs.DereferenceLevel;
  }
  return false;
}

bool ObjectNameDereference::operator!=(const ObjectNameDereference& rhs) const { return !(rhs == *this); }

StringType ObjectNameDereference::GetStringRepr() const {
  StringType Output;
  for (auto I = DereferenceLevel; I < 0; I++) {
    Output += "&";
  }
  for (auto I = 0; I < DereferenceLevel; I++) {
    Output += "*";
  }
  if (getOb()) {
    Output += getOb()->getStringRepr();
  } else if (getMb()) {
    Output += getMb()->GetStringRepr();
  } else {
    llvm_unreachable("No base");
  }
  return Output;
}

bool ObjectNameDereference::operator<(const ObjectNameDereference& Rhs) const {
  assert(getOb() || getMb());
  assert(Rhs.getOb() || Rhs.getMb());
  if (getOb() && Rhs.getOb()) {
    if (*getOb() < *Rhs.getOb()) {
      return true;
    }
    if (*Rhs.getOb() < *getOb()) {
      return false;
    }
    return DereferenceLevel < Rhs.DereferenceLevel;
  }
  if (getMb() && Rhs.getMb()) {
    if (*getMb() < *Rhs.getMb()) {
      return true;
    }
    if (*Rhs.getMb() < *getMb()) {
      return false;
    }
    return DereferenceLevel < Rhs.DereferenceLevel;
  }
  auto LOB = getOb();
  auto ROB = Rhs.getOb();
  auto LMB = getMb();
  auto RMB = Rhs.getMb();
  return std::tie(LOB, LMB, DereferenceLevel) < std::tie(ROB, RMB, Rhs.DereferenceLevel);
}

bool ObjectNameDereference::operator>(const ObjectNameDereference& rhs) const { return rhs < *this; }

bool ObjectNameDereference::operator<=(const ObjectNameDereference& rhs) const { return !(rhs < *this); }

bool ObjectNameDereference::operator>=(const ObjectNameDereference& rhs) const { return !(*this < rhs); }

std::optional<StringType> ObjectNameDereference::GetFunctionName() const {
  if (DereferenceLevel != 0 && !(DereferenceLevel == -1 && getOb())) {
    return {};
  }
  if (!getOb()) {
    return {};
  }
  return getOb()->GetFunctionName();
}

ObjectNameDereference::ObjectNameDereference(std::shared_ptr<ObjectName> OB, short DereferenceLevel)
    : OB(std::move(OB)), DereferenceLevel(DereferenceLevel) {
  assert(this->OB);
}

ObjectNameDereference::ObjectNameDereference(std::shared_ptr<ObjectNameMember> MB, short DereferenceLevel)
    : MB(std::move(MB)), DereferenceLevel(DereferenceLevel) {
  assert(this->MB);
}

Prefix::Prefix(const ObjectNameDereference& Object, StringType Member)
    : Object(Object.GetStringRepr()), Member(std::move(Member)) {}

StringType Prefix::GetStringRepr() const {
  StringType Output;
  Output += '-';
  if (!Member.empty()) {
    Output += Member;
  } else {
    Output += '*';
  }
  Output += "->";
  Output += Object;
  return Output;
}

bool Prefix::operator==(const Prefix& rhs) const { return Object == rhs.Object && Member == rhs.Member; }

bool Prefix::operator!=(const Prefix& rhs) const { return !(rhs == *this); }

bool Prefix::operator<(const Prefix& rhs) const {
  if (Object < rhs.Object)
    return true;
  if (rhs.Object < Object)
    return false;
  if (!Member.empty() && !rhs.Member.empty())
    return Member < rhs.Member;
  return !Member.empty();
}

bool Prefix::operator>(const Prefix& rhs) const { return rhs < *this; }

bool Prefix::operator<=(const Prefix& rhs) const { return !(rhs < *this); }

bool Prefix::operator>=(const Prefix& rhs) const { return !(*this < rhs); }

Prefix::Prefix(const ObjectNameDereference& Object) : Object(Object.GetStringRepr()) {}

Prefix::Prefix(const StringType& Object, const StringType& Member)
    : Object(std::move(Object)), Member(std::move(Member)) {}

FunctionInfo::FunctionInfo(clang::FunctionDecl* FD, clang::CXXMethodDecl* RealLambdaMethod) {
  if (FD->isVariadic()) {
    IsVariadic = true;
  }
  MangledNames = getMangledName(FD);
  for (const auto& Param : FD->parameters()) {
    const auto Decls = getDecls(Param, &FD->getASTContext());
    assert(Decls.size() == 1);
    Parameters.push_back(Decls[0]);
  }
  if (RealLambdaMethod) {
    if (RealLambdaMethod->hasBody()) {
      auto Body = RealLambdaMethod->getBody();
      ReferencedInReturnStmts = getReferencedInReturnStmts(Body, &FD->getASTContext(), RealLambdaMethod);
    }
  } else if (FD->hasBody()) {
    auto Body = FD->getBody();
    ReferencedInReturnStmts = getReferencedInReturnStmts(Body, &FD->getASTContext(), FD);
  }
  if (!llvm::isa<clang::CXXConstructorDecl>(FD) && !llvm::isa<clang::CXXDestructorDecl>(FD)) {
    ReferencedInReturnStmts.push_back(generateUSRForSymbolicReturn(FD));
  }
}

CallInfo::CallInfo(clang::CallExpr* CE, const clang::ASTContext* CTX, clang::FunctionDecl* ParentFunctionDecl,
                   bool IsOperatorMember) {
  CalledObjects = getReferencedDeclsStr(CE->getCallee(), CTX, ParentFunctionDecl);
  const auto Args = CE->getArgs();
  // Skip this pointer
  for (unsigned i = (IsOperatorMember ? 1 : 0); i < CE->getNumArgs(); i++) {
    Arguments.push_back(getReferencedDeclsStr(Args[i], CTX, ParentFunctionDecl));
  }
}

CallInfo::CallInfo(clang::CXXConstructExpr* CE, const clang::ASTContext* CTX, clang::FunctionDecl* ParentFunctionDecl) {
  CalledObjects.push_back(generateUSRForDecl(CE->getConstructor()));
  for (const auto Arg : CE->arguments()) {
    Arguments.push_back(getReferencedDeclsStr(Arg, CTX, ParentFunctionDecl));
  }
}

void EquivClassContainer::InitReferencedInCalls() {
  ReferencedInCalls.clear();
  for (auto CallInfo = CallInfoMap.cbegin(); CallInfo != CallInfoMap.cend(); CallInfo++) {
    const auto Refs = CallInfo->second.CalledObjects;
    for (const auto& Ref : Refs) {
      ReferencedInCalls[Ref].push_back(CallInfo);
    }
  }
}

EquivClass::EquivClass(StringType Obj) { Objects.emplace_back(std::move(Obj)); }

std::pair<VectorType<std::pair<StringType, CallInfoConstIterType>>, EquivClassesIterator> mergeRecurisve(
    EquivClassContainer& Data, EquivClassesIterator E1, EquivClassesIterator E2) {
  const auto MergeResult = merge(Data, E1, E2);
  VectorType<std::pair<StringType, CallInfoConstIterType>> Ret;

  // Merge function calls
  for (const auto& Entry : MergeResult.ObjectsToMerge) {
    auto F = Data.FindMap.find(Entry.first);
    auto F1 = Data.FindMap.find(Entry.second);
    assert(F != Data.FindMap.end());
    assert(F1 != Data.FindMap.end());
    if (F->second != F1->second) {
      const auto Tmp = mergeRecurisve(Data, F->second, F1->second);
      Ret.insert(Ret.end(), Tmp.first.begin(), Tmp.first.end());
    }
  }

  GetFunctionsToMerge(Data, Ret, *MergeResult.Iter->second);
  // We sadly can't reuse Iter here, as it can be invalidated by the merges of function calls
  return {Ret, MergeResult.Iter->second};
}

std::optional<std::pair<VectorType<std::pair<StringType, CallInfoConstIterType>>, std::pair<StringType, StringType>>>
mergeFunctionCall(EquivClassContainer& Data, const StringType& CalledObj, CallInfoConstIterType CE) {
  // TODO: A cache of already merged functions could be very useful here
  //  if (MergedCalls.find({CalledObj, CE}) != MergedCalls.end()) {
  //    return;
  //  }

  const auto FunctionMapIter = Data.FunctionMap.find(CalledObj);
  assert(FunctionMapIter != Data.FunctionMap.end());
  const auto CalledFunctionName = FunctionMapIter->second;
  const auto CallingFunctionName = Data.CallExprParentMap[CE->first];
  return mergeFunctionCallImpl(Data, CalledFunctionName, CallingFunctionName, CE->second, CE->first);
}

std::optional<std::pair<VectorType<std::pair<StringType, CallInfoConstIterType>>, std::pair<StringType, StringType>>>
mergeFunctionCallImpl(EquivClassContainer& Data, const StringType& CalledFunctionName,
                      const StringType& CallingFunctionName, const CallInfo& CI, const StringType& CallExprUSR) {
  VectorType<std::pair<StringType, CallInfoConstIterType>> Ret;
  assert(Data.FunctionInfoMap.find(CalledFunctionName) != Data.FunctionInfoMap.end());
  auto& CalledFunction = Data.FunctionInfoMap[CalledFunctionName];

  // Arguments
  unsigned int I = 0;
  const unsigned int NumArguments = CI.Arguments.size();
  for (const auto& Arg : CalledFunction.Parameters) {
    VectorType<StringType> RHS;
    if (I < NumArguments) {
      RHS = CI.Arguments[I];
    } else {
      // // TODO Handling of variadic arguments
      llvm::errs() << "Unhandled variadic argument in function: " << CalledFunctionName
                   << " Called from: " << CallingFunctionName << "\n";
    }

    const auto ItL = Data.FindMap.find(Arg);
    assert(ItL != Data.FindMap.end());
    for (const auto& R : RHS) {
      const auto ItR = Data.FindMap.find(R);

      assert(ItR != Data.FindMap.end());
      if (ItL->second != ItR->second) {
        const auto Tmp = mergeRecurisve(Data, ItL->second, ItR->second);
        Ret.insert(Ret.end(), Tmp.first.begin(), Tmp.first.end());
      }
    }

    I++;
  }

  // Returns
  const auto RHSObjects = CalledFunction.ReferencedInReturnStmts;
  if (!RHSObjects.empty()) {
    const ObjectName RetObj(CallExprUSR);
    const auto ItL = Data.FindMap.find(RetObj.getStringRepr());
    assert(ItL != Data.FindMap.end());
    for (const auto& R : RHSObjects) {
      const auto ItR = Data.FindMap.find(R);
      assert(ItR != Data.FindMap.end());
      if (ItL->second != ItR->second) {
        const auto Tmp = mergeRecurisve(Data, ItL->second, ItR->second);
        Ret.insert(Ret.end(), Tmp.first.begin(), Tmp.first.end());
      }
    }
  }
  //  MergedCalls.emplace(CalledObj, CE);
  return std::make_pair(Ret, std::make_pair(CallingFunctionName, CalledFunctionName));
}

void GetFunctionsToMerge(const EquivClassContainer& Data, VectorType<std::pair<StringType, CallInfoConstIterType>>& Ret,
                         const EquivClass& E) {
  VectorType<StringType> FunctionDecls;
  VectorType<CallInfoConstIterType> UsedInCall;
  for (const auto& Obj : E.Objects) {
    if (Data.FunctionMap.find(Obj) != Data.FunctionMap.end()) {
      FunctionDecls.emplace_back(Obj);
    }
    auto Calls = Data.ReferencedInCalls.find(Obj);
    if (Calls != Data.ReferencedInCalls.end()) {
      UsedInCall.insert(UsedInCall.end(), Calls->second.begin(), Calls->second.end());
    }
  }
  for (auto F : FunctionDecls) {
    for (auto C : UsedInCall) {
      Ret.emplace_back(F, C);
    }
  }
}

MergeResult merge(EquivClassContainer& Data, EquivClassesIterator E1, EquivClassesIterator E2) {
  assert(E1 != E2);
  assert(!E1->Objects.empty());
  assert(!E2->Objects.empty());

  VectorType<std::pair<StringType, StringType>> ObjectsToMerge;
  EquivClass E;

  E.Objects.reserve(E1->Objects.size() + E2->Objects.size());
  E.Objects.insert(E.Objects.end(), E1->Objects.begin(), E1->Objects.end());
  E.Objects.insert(E.Objects.end(), E2->Objects.begin(), E2->Objects.end());
  E.Prefixes = E1->Prefixes;
  const VectorType<Prefix> Prefix2 = E2->Prefixes;
  GetPrefixesToMerge(E, Prefix2, ObjectsToMerge);

  const auto Iter = Data.EquivClasses.insert(Data.EquivClasses.end(), E);
  for (const auto& Entry : E1->Objects) {
    Data.FindMap[Entry] = Iter;
  }
  for (const auto& Entry : E2->Objects) {
    Data.FindMap[Entry] = Iter;
  }

  Data.EquivClasses.erase(E1);
  Data.EquivClasses.erase(E2);

  // Make sure the prefixes are unique
  std::sort(E.Prefixes.begin(), E.Prefixes.end());
  E.Prefixes.erase(std::unique(E.Prefixes.begin(), E.Prefixes.end()), E.Prefixes.end());

  // We sadly can't reuse Iter here, as it can be invalidated by the merges of function calls
  return {Data.FindMap.find(E.Objects.front()), ObjectsToMerge};
}

void GetPrefixesToMerge(EquivClass& EquivClassToMergeInto, const VectorType<Prefix>& PrefixToMerge,
                        VectorType<std::pair<StringType, StringType>>& ObjectsToMerge) {
  for (const auto& P : PrefixToMerge) {
    const auto O1 = std::find_if(
        EquivClassToMergeInto.Prefixes.begin(), EquivClassToMergeInto.Prefixes.end(), [P](const Prefix& foundPrefix) {
          return (P.Object != foundPrefix.Object) &&
                 ((P.Member.empty() && foundPrefix.Member.empty()) ||
                  ((!P.Member.empty() && !foundPrefix.Member.empty()) && (P.Member == foundPrefix.Member)));
        });
    if (O1 != EquivClassToMergeInto.Prefixes.end()) {
      ObjectsToMerge.emplace_back(P.Object, O1->Object);
    } else {
      // In the algorithm as it is in the paper this is only done for cases where we can't do a recursive merge, this
      // leads to lost prefixes but is valid, as the lost prefixes would point to the same equivalence class. We are
      // doing it the same way, to keep the prefix class small
      EquivClassToMergeInto.Prefixes.push_back(P);
    }
  }
}

ASTInformationExtractor::InitInfo::InitInfo(ObjectNameDereference object, clang::Expr* init,
                                            clang::FunctionDecl* parentFunction)
    : Object(std::move(object)), Init(init), ParentFunction(parentFunction) {}
}  // namespace implementation
void calculateAliasInfo(clang::TranslationUnitDecl* TD, CallGraph* CG, nlohmann::json& J, int MetacgFormatVersion,
                        bool captureCtorsDtors, bool captureStackCtorsDtors) {
  assert(TD);
  assert(CG);
  implementation::ASTInformationExtractor Visitor(&TD->getASTContext(), CG, captureCtorsDtors, captureStackCtorsDtors);
  Visitor.TraverseDecl(TD);
  Visitor.calculatePERelation();
  // For debugging:
#ifdef DEBUG_TEST_AA
  // Visitor.dump();
#endif  // DEBUG_TEST_AA

  if (MetacgFormatVersion >= 2) {
    J = Visitor.getData();
  }
}
