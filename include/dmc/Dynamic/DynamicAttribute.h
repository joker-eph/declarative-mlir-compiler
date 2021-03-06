#pragma once

#include "Metadata.h"
#include "DynamicObject.h"
#include "dmc/Kind.h"
#include "dmc/Spec/ParameterList.h"

#include <mlir/IR/DialectImplementation.h>

namespace dmc {

/// Forward declarations.
class DynamicDialect;

namespace detail {
struct DynamicAttributeStorage;
} // end namespace detail

/// DynamicAttribute underlying class. Each dynamic Attribute instance holds
/// a reference to an instance of this class. Implementation details are
/// similar to DynamicType.
class DynamicAttributeImpl : public DynamicObject, public AttributeMetadata {
public:
  /// Create a dynamic attribute with the given name and parameter spec.
  explicit DynamicAttributeImpl(DynamicDialect *dialect, llvm::StringRef name,
                                NamedParameterRange paramSpec);

  /// Getters.
  inline DynamicDialect *getDialect() { return dialect; }
  inline auto getParamSpec() { return paramSpec; }

  /// Delegate parse and printer.
  mlir::Attribute parseAttribute(mlir::Location loc,
                                 mlir::DialectAsmParser &parser);
  void printAttribute(mlir::Attribute attr, mlir::DialectAsmPrinter &printer);
  void setFormat(std::string parserName, std::string printerName);

private:
  /// The dialect to which this attribute belongs.
  DynamicDialect *dialect;
  /// The dynamic attribute is formed by composing other attributes. The
  /// attributes must be Spec attributes.
  NamedParameterRange paramSpec;

  /// The function names of the custom parser and printer, if present.
  llvm::Optional<std::string> parserFcn, printerFcn;

  friend class DynamicAttribute;
};

class DynamicAttribute
    : public mlir::Attribute::AttrBase<DynamicAttribute, mlir::Attribute,
                                       detail::DynamicAttributeStorage> {
  /// Provide a single kind for casting to DynamicAttribute.
  static constexpr auto DynamicAttributeKind = dmc::Kind::FIRST_DYNAMIC_ATTR;

public:
  using Base::Base;

  /// Get a DynamicAttribute with a backing DynamicAttributeImpl and parameter
  /// values.
  static DynamicAttribute get(DynamicAttributeImpl *impl,
                              llvm::ArrayRef<mlir::Attribute> params);
  static DynamicAttribute getChecked(
    mlir::Location loc, DynamicAttributeImpl *impl,
    llvm::ArrayRef<mlir::Attribute> params);
  /// Verify that the parameter attributes are valid.
  static mlir::LogicalResult verifyConstructionInvariants(
    mlir::Location loc, DynamicAttributeImpl *impl,
    llvm::ArrayRef<mlir::Attribute> params);

  /// Allow casting Attribute to DynamicAttribute.
  static bool kindof(unsigned kind) { return kind == DynamicAttributeKind; }

  /// Getters.
  DynamicAttributeImpl *getDynImpl();
  llvm::ArrayRef<mlir::Attribute> getParams();
};

} // end namesdpace dmc
