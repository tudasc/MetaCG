#include "AAUSR.h"

//===- USRGeneration.cpp - Routines for USR generation --------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
// Copied and modified to support the generation of call expr 'USR

#include "clang/AST/ASTContext.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Index/USRGeneration.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/raw_ostream.h"

using namespace clang;
using namespace clang::index;

//===----------------------------------------------------------------------===//
// USR generation.
//===----------------------------------------------------------------------===//

/// \returns true on error.
/**
 * This function is a modified version of the function with the same name in clang. Used to generate a USR based on the
 * location of an expression
 * @param OS
 * @param BeginLoc
 * @param EndLoc
 * @param SM
 * @return
 */
bool implementation::printLoc(llvm::raw_ostream& OS, SourceLocation BeginLoc, SourceLocation EndLoc,
                              const SourceManager& SM, bool PrintFilename) {
  if (BeginLoc.isInvalid() || EndLoc.isInvalid()) {
    return true;
  }
  const auto BeginLocExpansion = SM.getExpansionLoc(BeginLoc);
  const auto EndLocExpansion = SM.getExpansionLoc(EndLoc);
  const auto EndLocOffset = SM.getDecomposedLoc(EndLocExpansion).second;
  const std::pair<FileID, unsigned>& Decomposed = SM.getDecomposedLoc(BeginLocExpansion);
  if (PrintFilename) {
    const FileEntry* FE = SM.getFileEntryForID(Decomposed.first);
    if (FE) {
      OS << llvm::sys::path::filename(FE->getName());
    } else {
      // This case really isn't interesting.
      return true;
    }
  }
  // Use the offest into the FileID to represent the location.  Using
  // a line/column can cause us to look back at the original source file,
  // which is expensive.
  OS << '@' << Decomposed.second << '-' << EndLocOffset;
  if (BeginLoc.isMacroID() || EndLoc.isMacroID()) {
    OS << '@';
    printLoc(OS, SM.getSpellingLoc(BeginLoc), SM.getSpellingLoc(EndLoc), SM, true);
  }
  clang::SourceLocation Loc;
  if (SM.isMacroArgExpansion(BeginLoc, &Loc)) {
    const auto Spelling = SM.getSpellingLoc(Loc);
    const auto ArgOffset = SM.getDecomposedLoc(Spelling).second;
    OS << '@' << ArgOffset;
  }

  return false;
}

void implementation::generateUSRForCallOrConstructExpr(const Expr* CE, const clang::ASTContext* Context,
                                                       SmallVectorImpl<char>& Buf,
                                                       clang::FunctionDecl* ParentFunctionDecl) {
  assert(llvm::isa<clang::CallExpr>(CE) || llvm::isa<clang::CXXConstructExpr>(CE));
  assert(ParentFunctionDecl);
  llvm::raw_svector_ostream Out(Buf);
  Out << generateUSRForDecl(ParentFunctionDecl);
  Out << "@CALL_EXPR@";
  [[maybe_unused]] const bool Error =
      printLoc(Out, CE->getBeginLoc(), CE->getEndLoc(), Context->getSourceManager(), false);
  assert(!Error);
}

StringType implementation::generateUSRForCallOrConstructExpr(const Expr* CE, const clang::ASTContext* Context,
                                                             clang::FunctionDecl* ParentFunctionDecl) {
  llvm::SmallString<64> Buffer;
  generateUSRForCallOrConstructExpr(CE, Context, Buffer, ParentFunctionDecl);
  return {Buffer.str().str()};
}

StringType implementation::generateUSRForDecl(const clang::Decl* DE) {
  assert(DE);
  llvm::SmallString<64> Buffer;
  const bool Ignore = clang::index::generateUSRForDecl(DE, Buffer);
  if (Ignore) {
    if (auto VD = llvm::dyn_cast<clang::ParmVarDecl>(DE)) {
      return generateUSRForUnnamedParamVarDecl(VD);
    } else if (auto FD = llvm::dyn_cast<clang::FieldDecl>(DE)) {
      if (FD->isAnonymousStructOrUnion()) {
        return generateUSRForUnnamedUnion(FD);
      } else {
        return generateUSRForUnnamedField(FD);
      }
    } else {
      assert(false && "Could not generate USR for declaration.");
    }
  }

  return {Buffer.str().str()};
}

StringType implementation::generateUSRForUnnamedParamVarDecl(const clang::ParmVarDecl* VD) {
  const auto Context = VD->getParentFunctionOrMethod();
  const auto F = llvm::dyn_cast_or_null<clang::FunctionDecl>(Context);
  assert(F);
  StringType Ret = generateUSRForDecl(F);
  Ret += "@UNNAMED_PARAM@";
  const auto Parameters = F->parameters();
  for (unsigned i = 0; i < F->getNumParams(); i++) {
    if (Parameters[i] == VD) {
      Ret += std::to_string(i);
      break;
    }
  }
  return Ret;
}

StringType implementation::generateUSRForUnnamedUnion(const clang::FieldDecl* FD) {
  const auto Parent = FD->getParent();
  assert(Parent);
  StringType Ret = generateUSRForDecl(Parent);
  Ret += "@UNNAMED_UNION";
  llvm::SmallString<64> Buffer;
  llvm::raw_svector_ostream Out(Buffer);
  [[maybe_unused]] const bool Error =
      printLoc(Out, Parent->getBeginLoc(), Parent->getEndLoc(), Parent->getASTContext().getSourceManager(), false);
  assert(!Error);
  Ret += Buffer.str();
  return Ret;
}

StringType implementation::generateUSRForUnnamedField(const clang::FieldDecl* FD) {
  const auto Parent = FD->getParent();
  assert(Parent);
  StringType Ret = generateUSRForDecl(Parent);
  Ret += "@UNNAMED_FIELD";
  llvm::SmallString<64> Buffer;
  llvm::raw_svector_ostream Out(Buffer);
  [[maybe_unused]] const bool Error =
      printLoc(Out, Parent->getBeginLoc(), Parent->getEndLoc(), Parent->getASTContext().getSourceManager(), false);
  assert(!Error);
  Ret += Buffer.str();
  return Ret;
}

StringType implementation::generateUSRForThisExpr(clang::FunctionDecl* ParentFunctionDecl) {
  assert(ParentFunctionDecl);
  auto USR = generateUSRForDecl(ParentFunctionDecl);
  USR += "@THIS";
  return USR;
}

StringType implementation::generateUSRForThisExpr(StringType ParentFunctionUSR) {
  ParentFunctionUSR += "@THIS";
  return ParentFunctionUSR;
}

StringType implementation::generateUSRForNewExpr(clang::CXXNewExpr* NE, clang::FunctionDecl* ParentFunctionDecl) {
  assert(ParentFunctionDecl);
  auto USR = generateUSRForDecl(ParentFunctionDecl);
  USR += "@NEW";
  llvm::SmallString<64> Buf;
  llvm::raw_svector_ostream Out(Buf);
  [[maybe_unused]] const bool Error =
      printLoc(Out, NE->getBeginLoc(), NE->getEndLoc(), ParentFunctionDecl->getASTContext().getSourceManager(), false);
  assert(!Error);
  USR += Buf.str();
  return USR;
}

StringType implementation::generateUSRForMaterializeTemporaryExpr(clang::MaterializeTemporaryExpr* MTE,
                                                                  clang::FunctionDecl* ParentFunctionDecl) {
  assert(ParentFunctionDecl);
  auto USR = generateUSRForDecl(ParentFunctionDecl);
  USR += "@MTE";
  llvm::SmallString<64> Buf;
  llvm::raw_svector_ostream Out(Buf);
  [[maybe_unused]] const bool Error = printLoc(Out, MTE->getBeginLoc(), MTE->getEndLoc(),
                                               ParentFunctionDecl->getASTContext().getSourceManager(), false);
  assert(!Error);
  USR += Buf.str();
  return USR;
}

StringType implementation::generateUSRForSymbolicReturn(clang::FunctionDecl* ParentFunctionDecl) {
  assert(ParentFunctionDecl);
  auto USR = generateUSRForDecl(ParentFunctionDecl);
  USR += "@SRETURN";
  return USR;
}

// TODO Typeid
// StringType implementation::generateUSRForTypeidExpr(const clang::CXXTypeidExpr *) { return "@TYPEID"; }
