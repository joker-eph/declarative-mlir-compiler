#include "dmc/Spec/SpecOps.h"

#include <mlir/IR/Builders.h>
#include <mlir/IR/OpImplementation.h>

using namespace mlir;

namespace dmc {

void addAttributesIfNotPresent(ArrayRef<NamedAttribute> attrs,
                               OperationState &result) {
  llvm::DenseSet<StringRef> present;
  present.reserve(result.attributes.size());
  for (auto &namedAttr : result.attributes)
    present.insert(namedAttr.first);
  for (auto &namedAttr : attrs)
    if (present.find(namedAttr.first) == std::end(present))
      result.attributes.push_back(namedAttr);
}

/// DialectOp.
void DialectOp::buildDefaultValuedAttrs(OpBuilder &builder,
                                        OperationState &result) {
  auto falseAttr = builder.getBoolAttr(false);
  addAttributesIfNotPresent({
      builder.getNamedAttr(getAllowUnknownOpsAttrName(), falseAttr),
      builder.getNamedAttr(getAllowUnknownTypesAttrName(), falseAttr)},
      result);
}

void DialectOp::build(OpBuilder &builder, OperationState &result,
                      StringRef name) {
  ensureTerminator(*result.addRegion(), builder, result.location);
  result.addAttribute(mlir::SymbolTable::getSymbolAttrName(),
      builder.getStringAttr(name));
  buildDefaultValuedAttrs(builder, result);
}

/// dialect-op ::= `Dialect` `@`symbolName `attributes` attr-list region
ParseResult DialectOp::parse(OpAsmParser &parser, OperationState &result) {
  mlir::StringAttr nameAttr;
  auto *body = result.addRegion();
  if (parser.parseSymbolName(nameAttr, mlir::SymbolTable::getSymbolAttrName(),
                             result.attributes) ||
      parser.parseOptionalAttrDictWithKeyword(result.attributes) ||
      parser.parseRegion(*body, llvm::None, llvm::None))
    return failure();
  OpBuilder builder{result.getContext()};
  buildDefaultValuedAttrs(builder, result);
  ensureTerminator(*body, parser.getBuilder(), result.location);
  return success();
}

void DialectOp::print(OpAsmPrinter &printer) {
  printer << getOperation()->getName() << ' ';
  printer.printSymbolName(getName());
  printer.printOptionalAttrDictWithKeyword(getAttrs(), {
      mlir::SymbolTable::getSymbolAttrName()});
  printer.printRegion(getBodyRegion(), /*printEntryBlockArgs=*/false,
                      /*printBlockTerminators=*/false);
}

LogicalResult DialectOp::verify() {
  auto &region = getBodyRegion();
  if (!llvm::hasSingleElement(region))
    return emitOpError("expected Dialect body to have one block");
  auto *body = getBody();
  if (body->getNumArguments() != 0)
    return emitOpError("expected Dialect body to have zero arguments");
  /// Verify attributes
  if (!getAttrOfType<BoolAttr>(getAllowUnknownOpsAttrName()))
    return emitOpError("expected BoolAttr named: ")
        << getAllowUnknownOpsAttrName();
  if (!getAttrOfType<BoolAttr>(getAllowUnknownTypesAttrName()))
    return emitOpError("expected BoolAttr named: ")
        << getAllowUnknownTypesAttrName();
  return success();
}

Region &DialectOp::getBodyRegion() { return getOperation()->getRegion(0); }
Block *DialectOp::getBody() { return &getBodyRegion().front(); }

StringRef DialectOp::getName() {
  return getAttrOfType<mlir::StringAttr>(mlir::SymbolTable::getSymbolAttrName())
      .getValue();
}

bool DialectOp::allowsUnknownOps() {
  return getAttrOfType<BoolAttr>(getAllowUnknownOpsAttrName()).getValue();
}

bool DialectOp::allowsUnknownTypes() {
  return getAttrOfType<BoolAttr>(getAllowUnknownTypesAttrName()).getValue();
}

/// OperationOp
void OperationOp::buildDefaultValuedAttrs(OpBuilder &builder,
                                          OperationState &result) {
  auto falseAttr = builder.getBoolAttr(false);
  addAttributesIfNotPresent({
      builder.getNamedAttr(getIsTerminatorAttrName(), falseAttr),
      builder.getNamedAttr(getIsCommutativeAttrName(), falseAttr),
      builder.getNamedAttr(getIsIsolatedFromAboveAttrName(), falseAttr)},
      result);
}

void OperationOp::build(OpBuilder &builder, OperationState &result,
                        StringRef name, FunctionType type,
                        ArrayRef<NamedAttribute> attrs) {
  result.addAttribute(SymbolTable::getSymbolAttrName(),
                      builder.getStringAttr(name));
  result.addAttribute(getTypeAttrName(), TypeAttr::get(type));
  result.attributes.append(std::begin(attrs), std::end(attrs));
  result.addRegion();
  buildDefaultValuedAttrs(builder, result);
}

// op ::= `dmc.Op` `@`opName type-list `->` type-list `attributes` attr-list
//        `config` attr-list
ParseResult OperationOp::parse(OpAsmParser &parser, OperationState &result) {
  mlir::StringAttr nameAttr;
  mlir::TypeAttr funcTypeAttr;
  mlir::DictionaryAttr opAttrs;
  if (parser.parseSymbolName(nameAttr, SymbolTable::getSymbolAttrName(),
                             result.attributes) ||
      parser.parseAttribute(funcTypeAttr, getTypeAttrName(),
                            result.attributes) ||
      parser.parseAttribute(opAttrs, getOpAttrDictAttrName(),
                            result.attributes))
    return failure();
  if (!parser.parseOptionalKeyword("config") &&
      parser.parseOptionalAttrDict(result.attributes))
    return failure();
  result.addRegion();
  OpBuilder builder{result.getContext()};
  buildDefaultValuedAttrs(builder, result);
  return success();
}

void OperationOp::print(OpAsmPrinter &printer) {
  printer << getOperation()->getName() << ' ';
  printer.printSymbolName(getName().getValue());
  printer.printType(getType());
  printer << ' ';
  printer.printAttribute(getOpAttrs());
  printer << " config";
  printer.printOptionalAttrDict(getAttrs(), {
      SymbolTable::getSymbolAttrName(), getTypeAttrName(),
      getOpAttrDictAttrName()});
}

mlir::StringAttr OperationOp::getName() {
  return getAttrOfType<mlir::StringAttr>(SymbolTable::getSymbolAttrName());
}

mlir::DictionaryAttr OperationOp::getOpAttrs() {
  return getAttrOfType<mlir::DictionaryAttr>(getOpAttrDictAttrName());
}

LogicalResult OperationOp::verify() {
  // TODO verify variadic arg conformance
  if (!getAttrOfType<BoolAttr>(getIsTerminatorAttrName()))
    return emitOpError("expected BoolAttr named: ")
        << getIsTerminatorAttrName();
  if (!getAttrOfType<BoolAttr>(getIsCommutativeAttrName()))
    return emitOpError("expected BoolAttr named: ")
        << getIsCommutativeAttrName();
  if (!getAttrOfType<BoolAttr>(getIsIsolatedFromAboveAttrName()))
    return emitOpError("expected BoolAttr named: ")
        << getIsIsolatedFromAboveAttrName();
  return success();
}

LogicalResult OperationOp::verifyType() {
  auto type = getTypeAttr().getValue();
  if (!type.isa<FunctionType>())
    return emitOpError("requires '" + getTypeAttrName() +
                       "' atttribute of function type");
  return success();
}

} // end namespace dmc
