#include "helper/common.h"
#include "clang/AST/DeclCXX.h"

#include <memory>

std::string getMangledName(clang::NamedDecl const *const nd) {
  // auto mc = nd->getASTContext().createMangleContext();
  std::unique_ptr<clang::MangleContext> mc(nd->getASTContext().createMangleContext());

  if (!nd || !mc) {
    std::cerr << "NamedDecl was nullptr" << std::endl;
    assert (nd && mc && "NamedDecl and MangleContext must not be nullptr");
    return "__NO_NAME__";
  }

  if (llvm::isa<clang::CXXConstructorDecl>(nd) || llvm::isa<clang::CXXDestructorDecl>(nd)) {
    std::string functionName;
    llvm::raw_string_ostream out(functionName);
    // FIXME: This code assumes that Ctors and Dtors are complete object constructors and destructors.
    // I do not know when this is the case / not the case. The clang code does not help me terrible much
    // in figuring it out.
    // See https://clang.llvm.org/doxygen/Mangle_8cpp_source.html
    if (llvm::isa<clang::CXXConstructorDecl>(nd)) {
      mc->mangleCXXCtor(llvm::dyn_cast<clang::CXXConstructorDecl>(nd), clang::CXXCtorType::Ctor_Complete, out);
    }
    if (llvm::isa<clang::CXXDestructorDecl>(nd)) {
      mc->mangleCXXDtor(llvm::dyn_cast<clang::CXXDestructorDecl>(nd), clang::CXXDtorType::Dtor_Complete, out);
    }
    return out.str();
  }

  if (const clang::FunctionDecl *dc = llvm::dyn_cast<clang::FunctionDecl>(nd)) {
    if (dc->isExternC()) {
      return dc->getNameAsString();
    }
    if (dc->isMain()) {
      return "main";
    }
  }
  std::string functionName;
  llvm::raw_string_ostream llvmName(functionName);
  mc->mangleName(nd, llvmName);
  return llvmName.str();
}
