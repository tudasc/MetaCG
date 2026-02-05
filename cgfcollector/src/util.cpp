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
    mangledName = GetExternalAssemblyName(result.second.name, true);
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

bool compareExprIntrinsicOperator(const Expr* expr, DefinedOperator::IntrinsicOperator op) {
  if (!expr)
    return false;

  using IO = DefinedOperator::IntrinsicOperator;

  switch (op) {
    case IO::NOT:
      return std::get_if<Expr::NOT>(&expr->u) != nullptr;
    case IO::Power:
      return std::get_if<Expr::Power>(&expr->u) != nullptr;
    case IO::Multiply:
      return std::get_if<Expr::Multiply>(&expr->u) != nullptr;
    case IO::Divide:
      return std::get_if<Expr::Divide>(&expr->u) != nullptr;
    case IO::Add:
      return std::get_if<Expr::Add>(&expr->u) != nullptr ||
             std::get_if<Expr::UnaryPlus>(&expr->u) != nullptr;  // UnaryPlus also uses +
    case IO::Subtract:
      return std::get_if<Expr::Subtract>(&expr->u) != nullptr ||
             std::get_if<Expr::Negate>(&expr->u) != nullptr;  // Negate also uses -
    case IO::Concat:
      return std::get_if<Expr::Concat>(&expr->u) != nullptr;
    case IO::LT:
      return std::get_if<Expr::LT>(&expr->u) != nullptr;
    case IO::LE:
      return std::get_if<Expr::LE>(&expr->u) != nullptr;
    case IO::EQ:
      return std::get_if<Expr::EQ>(&expr->u) != nullptr;
    case IO::NE:
      return std::get_if<Expr::NE>(&expr->u) != nullptr;
    case IO::GE:
      return std::get_if<Expr::GE>(&expr->u) != nullptr;
    case IO::GT:
      return std::get_if<Expr::GT>(&expr->u) != nullptr;
    case IO::AND:
      return std::get_if<Expr::AND>(&expr->u) != nullptr;
    case IO::OR:
      return std::get_if<Expr::OR>(&expr->u) != nullptr;
    case IO::EQV:
      return std::get_if<Expr::EQV>(&expr->u) != nullptr;
    case IO::NEQV:
      return std::get_if<Expr::NEQV>(&expr->u) != nullptr;
    default:
      return false;
  }
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

template <typename Variant>
DefinedOperator::IntrinsicOperator variantGetIntrinsicOperator(const Variant& op) {
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
                    op);
}

const Symbol* getTypeSymbolFromSymbol(const Symbol* symbol) {
  auto* type = symbol->GetType();
  if (!type)
    return nullptr;
  auto* derived = type->AsDerived();
  if (!derived)
    return nullptr;
  auto* typeSymbol = &derived->typeSymbol();
  if (!typeSymbol)
    return nullptr;
  return typeSymbol;
}
