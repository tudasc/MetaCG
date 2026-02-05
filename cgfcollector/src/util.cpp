#include "util.h"

// Compares two symbols for equality also resolves the original contruct the symbol comes from. This could have been
// defined in another module/file
bool compareSymbols(const Symbol* a, const Symbol* b) {
  if (a == b)
    return true;
  if (!a || !b)
    return false;
  if (a->name() != b->name())
    return false;

  auto resolveHostAssoc = [](const Symbol* sym) -> const Symbol* {
    while (sym) {
      if (sym->has<HostAssocDetails>()) {
        sym = &sym->get<HostAssocDetails>().symbol();
      } else if (sym->has<UseDetails>()) {
        sym = &sym->get<UseDetails>().symbol();
      } else {
        break;
      }
    }
    return sym;
  };
  if (resolveHostAssoc(a) != resolveHostAssoc(b))
    return false;

  if (a->attrs() != b->attrs())
    return false;

  // this only compares only the type and not all details like variables, procedures, generics, etc. But should be
  // enough for now.
  if (a->GetType() != b->GetType())
    return false;

  return true;
}

std::string mangleSymbol(const Symbol* sym) {
  assert(sym && "mangleSymbol called with nullptr");

  std::string mangledName = mangleName(*sym);

  // Legacy Fortran - C interoperability before BIND(C) existed.
  // We have to do this manually because normally it would run as
  // a pass (ExternalNameConversionPass).
  //
  // NOTE: underscoring can be disabled with `-fno-underscoring`
  auto result = fir::NameUniquer::deconstruct(mangledName);
  if (fir::NameUniquer::isExternalFacingUniquedName(result)) {
    if (result.first == fir::NameUniquer::NameKind::COMMON && result.second.name.empty())
      mangledName = Fortran::common::blankCommonObjectName;
    mangledName = Fortran::common::GetExternalAssemblyName(result.second.name, true);
  }

  return mangledName;
}
