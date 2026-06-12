/**
 * File: FortranUtil.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "FortranUtil.h"

#include <flang/Semantics/symbol.h>

using namespace Fortran::semantics;
using namespace Fortran::parser;
using namespace Fortran::common;
using namespace metacg;

namespace metacg::cgfcollector {

const Symbol* getTypeAsDerivedTypeSymbol(const Symbol* sym) {
  if (auto* type = sym->GetType()) {
    if (auto* derived = type->AsDerived()) {
      return &derived->typeSymbol();
    }
  }
  return nullptr;
}

const Symbol* getAbsoluteBaseSymbol(const std::vector<Type>& types, const Type* type) {
  const Symbol* base = type->typeSymbol;
  const Symbol* current = type->extendsFrom;
  while (current) {
    base = current;
    auto it = std::find_if(types.begin(), types.end(), [&](const Type& t) { return t.typeSymbol == current; });
    if (it == types.end())
      break;
    current = it->extendsFrom;
  }
  return canonicalizeSymbol(base).symbol;
}

/**
 * @brief Resolve a symbol by following UseDetails and HostAssocDetails until we reach a symbol that has neither. This
 * is used to canonicalize symbols.
 *
 * @param sym
 * @return
 */
static const Symbol* resolveUseHostSymbol(const Symbol* sym) {
  while (true) {
    if (auto* host = sym->detailsIf<HostAssocDetails>()) {
      sym = &host->symbol();
      continue;
    }
    if (auto* use = sym->detailsIf<UseDetails>()) {
      sym = &use->symbol();
      continue;
    }
    break;
  }
  return sym;
}

CanonicalSymbol canonicalizeSymbol(const Symbol* input, CanonicalMode mode) {
  std::unordered_set<const Symbol*> visited;
  const Symbol* sym = input;

  while (sym && !visited.count(sym)) {
    visited.insert(sym);
    sym = resolveUseHostSymbol(sym);

    // DerivedTypeDetails, is already canonicalized.
    if (sym->detailsIf<DerivedTypeDetails>()) {
      return {sym, CanonicalSymbol::Kind::DerivedType};
    }

    // GenericDetails, unpack to specific procedure if it has one. This is forwarded to the next check for
    // SubprogramDetails. TODO: here we ignore possibly multiple specific procedure definitions. For the comparison step
    // these need to be also considered. That's why this falls short when using generics.
    if (auto* gen = sym->detailsIf<GenericDetails>()) {
      if (!gen->specificProcs().empty()) {
        const Symbol* proc = &gen->specificProcs().front().get();
        sym = proc;
        continue;
      }
    }

    // SubprogramDetails, is already canonicalized.
    if (auto* sub = sym->detailsIf<SubprogramDetails>()) {
      // If it's a function, it was likely unpacked from the GenericDetails. We then extract the derived type symbol
      // from the result type of the function, if it exists. This is mainly to handle cases where the definition of a
      // constructor (defined through an interface block) with the same name as the derived type is defined. In LLVM
      // versions 18 and before, the constructor would shadow the derived type symbol, leading that type beging lost in
      // the process. In LLVM 19, this limitation is fixed, meaning constructors and derived types can better
      // differentiated. We keep this for backwards compatibility.
      if (sub->isFunction()) {
        const Symbol& resultSym = sub->result();
        if (const auto* typeSym = getTypeAsDerivedTypeSymbol(&resultSym)) {
          sym = typeSym;
          continue;
        }
      }
      return {sym, CanonicalSymbol::Kind::Procedure};
    }

    // If in ByType mode, unpack symbol type to the derived type symbol T, if it exists. This is handled in an extra
    // mode because if we compare variables with the derived type definition we need to get to the type with GetType, to
    // compare the actual type. Trying and comparing variables with the derived type definition would not work and makes
    // no sense. But other times we don't want to unpack to the type but keep the symbol as is, and compare the derived
    // type symbols directly.
    if (mode == CanonicalMode::ByType) {
      if (const auto* typeSym = getTypeAsDerivedTypeSymbol(sym)) {
        sym = typeSym;
        continue;
      }
    }

    return {sym, CanonicalSymbol::Kind::Other};
  }

  return {sym, CanonicalSymbol::Kind::Other};
}

bool compareSymbols(const Symbol* a, const Symbol* b, CanonicalMode mode) {
  auto ca = canonicalizeSymbol(a, mode);
  auto cb = canonicalizeSymbol(b, mode);

  MCGLogger::logDebug("Comparing symbols: {} ({}) ({}) and {} ({}) ({}), canon: {} ({}) ({}) {} ({}) ({})",
                      a ? a->name() : "nullptr", a ? getDetailsName(a) : "nullptr", fmt::ptr(a),
                      b ? b->name() : "nullptr", b ? getDetailsName(b) : "nullptr", fmt::ptr(b),
                      ca.symbol ? ca.symbol->name() : "nullptr", ca.symbol ? getDetailsName(ca.symbol) : "nullptr",
                      fmt::ptr(ca.symbol), cb.symbol ? cb.symbol->name() : "nullptr",
                      cb.symbol ? getDetailsName(cb.symbol) : "nullptr", fmt::ptr(cb.symbol));

  return ca.kind == cb.kind && ca.symbol == cb.symbol;
}

std::string mangleSymbol(const Symbol* sym, bool underscoring) {
  assert(sym && "mangleSymbol called with nullptr");

  std::string mangledName = Fortran::lower::mangle::mangleName(*sym);

  // Legacy Fortran - C interoperability before BIND(C) existed.
  // We have to do this manually because normally it would run as
  // a pass (ExternalNameConversionPass).
  //
  // NOTE: underscoring can be disabled with `-fno-underscoring`
  auto result = fir::NameUniquer::deconstruct(mangledName);
  if (fir::NameUniquer::isExternalFacingUniquedName(result)) {
    if (result.first == fir::NameUniquer::NameKind::COMMON && result.second.name.empty())
      mangledName = blankCommonObjectName;
    mangledName = GetExternalAssemblyName(result.second.name, underscoring);
  }

  return mangledName;
}

bool isOperator(const Expr* e) {
  /* Operators: see 15.4.3.4.2, 10.1.6.1, 6.2.4 (https://j3-fortran.org/doc/year/23/23-007r1.pdf)
     Negate, NOT, Power, Multiply, Divide, Add, Subtract, Concat,
     LT, LE, EQ, NE, GE, GT, AND, OR, EQV, NEQV,
     DefinedUnary, DefinedBinary
   */

  return holds_any_of<decltype(e->u), Expr::UnaryPlus, Expr::Negate, Expr::NOT, Expr::Power, Expr::Multiply,
                      Expr::Divide, Expr::Add, Expr::Subtract, Expr::Concat, Expr::LT, Expr::LE, Expr::EQ, Expr::NE,
                      Expr::GE, Expr::GT, Expr::AND, Expr::OR, Expr::EQV, Expr::NEQV, Expr::DefinedUnary,
                      Expr::DefinedBinary>(e->u);
}

std::string getOperatorStringFromExpr(const Expr* expr) {
  if (!isOperator(expr)) {
    return "";
  }

  return std::visit(visitors{[](const Expr::UnaryPlus&) { return std::string("ADD"); },
                             [](const Expr::Negate&) { return std::string("SUBTRACT"); },
                             [](const Expr::NOT&) { return std::string("NOT"); },
                             [](const Expr::Power&) { return std::string("POWER"); },
                             [](const Expr::Multiply&) { return std::string("MULTIPLY"); },
                             [](const Expr::Divide&) { return std::string("DIVIDE"); },
                             [](const Expr::Add&) { return std::string("ADD"); },
                             [](const Expr::Subtract&) { return std::string("SUBTRACT"); },
                             [](const Expr::Concat&) { return std::string("CONCAT"); },
                             [](const Expr::LT&) { return std::string("LT"); },
                             [](const Expr::LE&) { return std::string("LE"); },
                             [](const Expr::EQ&) { return std::string("EQ"); },
                             [](const Expr::NE&) { return std::string("NE"); },
                             [](const Expr::GE&) { return std::string("GE"); },
                             [](const Expr::GT&) { return std::string("GT"); },
                             [](const Expr::AND&) { return std::string("AND"); },
                             [](const Expr::OR&) { return std::string("OR"); },
                             [](const Expr::EQV&) { return std::string("EQV"); },
                             [](const Expr::NEQV&) { return std::string("NEQV"); },
                             [](const Expr::DefinedUnary& op) {
                               auto out = std::get<DefinedOpName>(op.t).v.ToString();
                               std::transform(out.begin(), out.end(), out.begin(), ::toupper);
                               return out;
                             },
                             [](const Expr::DefinedBinary& op) {
                               auto out = std::get<DefinedOpName>(op.t).v.ToString();
                               std::transform(out.begin(), out.end(), out.begin(), ::toupper);
                               return out;
                             },
                             [](const auto&) -> std::string { return std::string("UNKNOWN_OPERATOR"); }},
                    expr->u);
}

std::string getOperatorStringFromDefinedOperator(const DefinedOperator* op) {
  std::string out = std::visit(visitors{[](const DefinedOperator::IntrinsicOperator& v) {
                                          return std::string(DefinedOperator::EnumToString(v));
                                        },
                                        [](const DefinedOpName& v) { return v.v.ToString(); },
                                        [](const auto&) { return std::string("UNKNOWN_DEFINED_OPERATOR"); }},
                               op->u);

  std::transform(out.begin(), out.end(), out.begin(), ::toupper);
  return out;
}

std::string getOperatorStringFromGenericDetails(const Symbol* symbol, const GenericKind& gk) {
  std::string out = "UNKNOWN_GENERIC_KIND";

  if (gk.IsIntrinsicOperator()) {
    out = DefinedOperator::EnumToString(variantGetIntrinsicOperator(gk));
  }

  if (gk.IsDefinedOperator()) {
    out = symbol->name().ToString();
  }

  std::transform(out.begin(), out.end(), out.begin(), ::toupper);
  return out;
}

bool compareExprIntrinsicOperator(const Expr* expr, DefinedOperator::IntrinsicOperator op) {
  if (!expr)
    return false;

  return std::visit(
      visitors{[&](const Expr::UnaryPlus& e) { return op == DefinedOperator::IntrinsicOperator::Add; },
               [&](const Expr::Negate& e) { return op == DefinedOperator::IntrinsicOperator::Subtract; },
               [&](const Expr::NOT& e) { return op == DefinedOperator::IntrinsicOperator::NOT; },
               [&](const Expr::Power& e) { return op == DefinedOperator::IntrinsicOperator::Power; },
               [&](const Expr::Multiply& e) { return op == DefinedOperator::IntrinsicOperator::Multiply; },
               [&](const Expr::Divide& e) { return op == DefinedOperator::IntrinsicOperator::Divide; },
               [&](const Expr::Add& e) { return op == DefinedOperator::IntrinsicOperator::Add; },
               [&](const Expr::Subtract& e) { return op == DefinedOperator::IntrinsicOperator::Subtract; },
               [&](const Expr::Concat& e) { return op == DefinedOperator::IntrinsicOperator::Concat; },
               [&](const Expr::LT& e) { return op == DefinedOperator::IntrinsicOperator::LT; },
               [&](const Expr::LE& e) { return op == DefinedOperator::IntrinsicOperator::LE; },
               [&](const Expr::EQ& e) { return op == DefinedOperator::IntrinsicOperator::EQ; },
               [&](const Expr::NE& e) { return op == DefinedOperator::IntrinsicOperator::NE; },
               [&](const Expr::GE& e) { return op == DefinedOperator::IntrinsicOperator::GE; },
               [&](const Expr::GT& e) { return op == DefinedOperator::IntrinsicOperator::GT; },
               [&](const Expr::AND& e) { return op == DefinedOperator::IntrinsicOperator::AND; },
               [&](const Expr::OR& e) { return op == DefinedOperator::IntrinsicOperator::OR; },
               [&](const Expr::EQV& e) { return op == DefinedOperator::IntrinsicOperator::EQV; },
               [&](const Expr::NEQV& e) { return op == DefinedOperator::IntrinsicOperator::NEQV; },

               [](const auto&) { return false; }},
      expr->u);
}

bool isBinaryOperator(const Expr* e) {
  if (!e)
    return false;

  return holds_any_of<decltype(e->u), Expr::Power, Expr::Multiply, Expr::Divide, Expr::Add, Expr::Subtract,
                      Expr::Concat, Expr::LT, Expr::LE, Expr::EQ, Expr::NE, Expr::GE, Expr::GT, Expr::AND, Expr::OR,
                      Expr::EQV, Expr::NEQV, Expr::DefinedBinary>(e->u);
}

bool isUnaryOperator(const Expr* e) {
  if (!e)
    return false;

  return holds_any_of<decltype(e->u), Expr::UnaryPlus, Expr::Negate, Expr::NOT, Expr::DefinedUnary>(e->u);
}

DefinedOperator::IntrinsicOperator variantGetIntrinsicOperator(const GenericKind& gk) {
  return std::visit(visitors{[](const RelationalOperator& op) {
                               using RO = RelationalOperator;
                               using IO = DefinedOperator::IntrinsicOperator;

                               switch (op) {
                                 case RO::LT:
                                   return IO::LT;
                                 case RO::LE:
                                   return IO::LE;
                                 case RO::EQ:
                                   return IO::EQ;
                                 case RO::NE:
                                   return IO::NE;
                                 case RO::GE:
                                   return IO::GE;
                                 case RO::GT:
                                   return IO::GT;
                                 default:
                                   MCGLogger::logDebug("Error: Unknown RelationalOperator in mapToIntrinsicOperator");
                                   return IO::LT;  // avoid warning
                               }
                             },
                             [](const LogicalOperator& op) {
                               using LO = LogicalOperator;
                               using IO = DefinedOperator::IntrinsicOperator;

                               switch (op) {
                                 case LO::And:
                                   return IO::AND;
                                 case LO::Or:
                                   return IO::OR;
                                 case LO::Eqv:
                                   return IO::EQV;
                                 case LO::Neqv:
                                   return IO::NEQV;
                                 case LO::Not:
                                   return IO::NOT;
                                 default:
                                   MCGLogger::logDebug("Error: Unknown LogicalOperator in mapToIntrinsicOperator");
                                   return IO::AND;  // avoid warning
                               }
                             },
                             [](const NumericOperator& op) {
                               using NO = NumericOperator;
                               using IO = DefinedOperator::IntrinsicOperator;

                               switch (op) {
                                 case NO::Power:
                                   return IO::Power;
                                 case NO::Multiply:
                                   return IO::Multiply;
                                 case NO::Divide:
                                   return IO::Divide;
                                 case NO::Add:
                                   return IO::Add;
                                 case NO::Subtract:
                                   return IO::Subtract;
                                 default:
                                   MCGLogger::logDebug("Error: Unknown NumericOperator in mapToIntrinsicOperator");
                                   return IO::Add;  // avoid warning
                               }
                             },
                             [](const auto& op) {
                               MCGLogger::logDebug("Error: Unknown operator type in mapToIntrinsicOperator");
                               return DefinedOperator::IntrinsicOperator::Add;  // avoid warning
                             }},
                    gk.u);
}

std::string getDetailsName(const Symbol* symbol) {
  assert(symbol && "getDetailsName called with nullptr");

  const char* names[] = {"UnknownDetails",        "MainProgramDetails", "ModuleDetails",       "SubprogramDetails",
                         "SubprogramNameDetails", "EntityDetails",      "ObjectEntityDetails", "ProcEntityDetails",
                         "AssocEntityDetails",    "DerivedTypeDetails", "UseDetails",          "UseErrorDetails",
                         "HostAssocDetails",      "GenericDetails",     "ProcBindingDetails",  "NamelistDetails",
                         "CommonBlockDetails",    "TypeParamDetails",   "MiscDetails",         "UserReductionDetails",
                         "MapperDetails"};
  return names[symbol->details().index()];
}

}  // namespace metacg::cgfcollector
