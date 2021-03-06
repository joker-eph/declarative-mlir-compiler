#include "Utility.h"
#include "Identifier.h"
#include "AsmUtils.h"
#include "dmc/Python/OpAsm.h"
#include "dmc/Traits/SpecTraits.h"

#include <mlir/IR/OpImplementation.h>
#include <pybind11/pybind11.h>

using namespace pybind11;
using namespace mlir;
using namespace llvm;

using dmc::py::OperationWrap;

namespace dmc {
namespace py {
extern void exposeOperationWrap(module &m);
extern void exposeResultWrap(module &m);
extern void exposeDynamicTypes(module &m);
} // end namespace py
} // end namespace dmc

namespace mlir {
namespace py {

namespace {

auto transformAttrStorage(AttrDictRef attrs, StringListRef elidedAttrs) {
  /// Convert unordered_map<string, Attribute> -> vector<{Identifier, Attribute}>
  /// and vector<string> -> vector<StringRef>
  NamedAttrList namedAttrs;
  for (auto &[name, attr] : attrs)
    namedAttrs.push_back({getIdentifierChecked(name), attr});
  std::vector<StringRef> refs;
  refs.reserve(std::size(elidedAttrs));
  for (auto &elidedAttr : elidedAttrs)
    refs.emplace_back(elidedAttr);
  return std::make_pair(std::move(namedAttrs), std::move(refs));
}

template <typename E> bool is_list_of(const list &vals) {
  return llvm::all_of(vals, [](auto &val) { return isinstance<E>(val); });
}

template <typename E> auto map_list(const list &vals) {
  return llvm::map_range(vals, [](auto &val) { return val.template cast<E>(); });
}

void genericPrint(OpAsmPrinter &printer, const object &obj) {
  if (isinstance<Value>(obj)) {
    printer.printOperand(obj.cast<Value>());
  } else if (isinstance<list>(obj)) {
    auto vals = obj.cast<list>();
    if (is_list_of<Value>(vals)) {
      printer.printOperands(map_list<Value>(vals));
    } else if (is_list_of<Type>(vals)) {
      llvm::interleaveComma(map_list<Type>(vals), printer);
    } else {
      llvm::interleaveComma(vals, printer,
                            [&](auto &val) { genericPrint(printer, obj); });
    }
  } else if (isinstance<iterator>(obj)) {
    /// An iterator can only be exhausted once and the internet state cannot be
    /// copied, so empty its contents into a list.
    auto it = obj.cast<iterator>();
    list itList;
    while (it != iterator::sentinel())
      itList.append(*it++);
    genericPrint(printer, itList);
  } else if (isinstance<Type>(obj)) {
    printer.printType(obj.cast<Type>());
  } else if (isinstance<Attribute>(obj)) {
    printer.printAttribute(obj.cast<Attribute>());
  } else if (isinstance<bool>(obj)) {
    printer << obj.cast<bool>();
  } else if (isinstance<Block>(obj)) {
    printer.printSuccessor(&obj.cast<Block &>());
  } else {
    std::string repr = str(obj);
    printer << repr;
  }
}

static LogicalResult resolveOperands(OpAsmParser &parser, list operands,
                                     list types, OperationState &result) {
  for (auto [operand, type] : llvm::zip(operands, types)) {
    if (parser.resolveOperand(operand.cast<OpAsmParser::OperandType>(),
                              type.cast<Type>(), result.operands))
      return failure();
  }
  return success();
}

} // end anonymous namespace

void exposeOpAsm(module &m) {
  class_<OpAsmPrinter, std::unique_ptr<OpAsmPrinter, nodelete>>(m, "OpAsmPrinter")
      .def("printOperand", overload<void(OpAsmPrinter::*)(Value)>(
          &OpAsmPrinter::printOperand))
      .def("printOperands", [](OpAsmPrinter &printer, ValueRange values) {
        printer.printOperands(values);
      })
      .def("printType", &OpAsmPrinter::printType)
      .def("printTypes", [](OpAsmPrinter &printer, list types) {
        llvm::interleaveComma(types, printer, [&](handle type)
                              { printer.printType(type.cast<Type>()); });
      })
      .def("printAttribute", &OpAsmPrinter::printAttribute)
      .def("printAttributeWithoutType", &OpAsmPrinter::printAttributeWithoutType)
      .def("printSymbolName", [](OpAsmPrinter &printer, std::string name) {
        printer.printSymbolName(name);
      })
      .def("printSuccessor", &OpAsmPrinter::printSuccessor)
      .def("printSuccessorAndUseList", [](OpAsmPrinter &printer, Block *successor,
                                          ValueListRef succOperands) {
        printer.printSuccessorAndUseList(successor, succOperands);
      })
      .def("printOptionalAttrDict", [](OpAsmPrinter &printer, AttrDictRef attrs,
                                       StringListRef elidedAttrs) {
        auto [namedAttrs, refs] = transformAttrStorage(attrs, elidedAttrs);
        printer.printOptionalAttrDict(namedAttrs, refs);
      }, "attrs"_a, "elidedAttrs"_a = StringList{})
      .def("printOptionalAttrDictWithKeyword",
           [](OpAsmPrinter &printer, AttrDictRef attrs, StringListRef elidedAttrs) {
        auto [namedAttrs, refs] = transformAttrStorage(attrs, elidedAttrs);
        printer.printOptionalAttrDictWithKeyword(namedAttrs, refs);
      }, "attrs"_a, "elidedAttrs"_a = StringList{})
      .def("printGenericOp", &OpAsmPrinter::printGenericOp)
      .def("printRegion", &OpAsmPrinter::printRegion, "region"_a,
           "printEntryBlockArgs"_a = true, "printBlockTerminators"_a = true)
      .def("printBlockArguments", [](OpAsmPrinter &printer, Block &block) {
        llvm::interleaveComma(block.getArguments(), printer, [&](Value &val) {
          printer << val << ": " << val.getType();
        });
      })
      .def("shadowRegionArgs", [](OpAsmPrinter &printer, Region &region,
                                  ValueListRef namesToUse) {
        printer.shadowRegionArgs(region, namesToUse);
      })
      .def("printAffineMapOfSSAIds", [](OpAsmPrinter &printer, AffineMapAttr mapAttr,
                                        ValueListRef operands) {
        printer.printAffineMapOfSSAIds(mapAttr, operands);
      })
      .def("printOptionalArrowTypeList",
           &OpAsmPrinter::printOptionalArrowTypeList<TypeListRef>)
      .def("printArrowTypeList", &OpAsmPrinter::printArrowTypeList<TypeListRef>)
      .def("printFunctionalType",
           overload<void(OpAsmPrinter::*)(Operation *)>(&OpAsmPrinter::printFunctionalType))
      .def("printFunctionalType",
           &OpAsmPrinter::printFunctionalType<TypeListRef, TypeListRef>)
      .def("print", [](OpAsmPrinter &printer, pybind11::args args) {
        for (auto &val : args)
          genericPrint(printer, reinterpret_borrow<object>(val));
      });

  class_<OpAsmParser, std::unique_ptr<OpAsmParser, nodelete>>
      parserCls{m, "OpAsmParser"};
  parserCls
      .def("getCurrentLocation",
           overload<SMLoc(OpAsmParser::*)()>(&OpAsmParser::getCurrentLocation))
      .def("parseOperand", [](OpAsmParser &parser) {
        OpAsmParser::OperandType operand;
        auto ret = parser.parseOperand(operand);
        return make_tuple(operand, ret);
      })
      .def("parseAttribute", [](OpAsmParser &parser, OperationState &result,
                                std::string name, Type type) {
        Attribute attr;
        return parser.parseAttribute(attr, type, name, result.attributes);
      }, "attributes"_a, "name"_a, "type"_a = Type{})
      .def("parseSymbolName", [](OpAsmParser &parser, OperationState &result,
                                 std::string name) {
        StringAttr attr;
        return parser.parseSymbolName(attr, name, result.attributes);
      })
      .def("parseOptionalAttrDict", [](OpAsmParser &parser,
                                       OperationState &result) {
        return parser.parseOptionalAttrDict(result.attributes);
      })
      .def("parseOptionalAttrDictWithKeyword", [](OpAsmParser &parser,
                                                  OperationState &result) {
        return parser.parseOptionalAttrDictWithKeyword(result.attributes);
      })
      .def("parseType", [](OpAsmParser &parser) {
        Type type;
        auto ret = parser.parseType(type);
        return make_tuple(type, ret);
      })
      .def("resolveOperands", [](OpAsmParser &parser, list operands, list types,
                                 SMLoc loc, OperationState &result)
                              -> LogicalResult {
        if (operands.size() != types.size())
          return parser.emitError(loc) << operands.size()
              << " operands present, but expected " << types.size();
        return resolveOperands(parser, operands, types, result);
      })
      .def("resolveOperands", [](OpAsmParser &parser, list operands, list types,
                                 OperationState &result) -> LogicalResult {
        if (operands.size() != types.size())
          return failure();
        return resolveOperands(parser, operands, types, result);
      })
      .def("parseFunctionalType", [](OpAsmParser &parser) {
        FunctionType funcType;
        auto ret = parser.parseType(funcType);
        return make_tuple(funcType, ret);
      })
      .def("parseKeyword", [](OpAsmParser &parser, std::string keyword) {
        return parser.parseKeyword(keyword);
      })
      .def("parseOptionalKeyword", [](OpAsmParser &parser,
                                      std::string keyword) {
        return parser.parseOptionalKeyword(keyword);
      })
      .def("parseOperandList", [](OpAsmParser &parser) {
        // Returned list may interface with result.operands or a Python list,
        // so we must copy into a py::list.
        list operandList;
        SmallVector<OpAsmParser::OperandType, 4> operands;
        if (failed(parser.parseOperandList(operands)))
          return make_tuple(operandList, failure());
        for (auto &operand : operands)
          operandList.append(pybind11::cast(operand));
        return make_tuple(operandList, success());
      })
      .def("parseTypeList", [](OpAsmParser &parser) {
        // Returned list may interface with result.types or a Python list.
        list typeList;
        SmallVector<Type, 4> types;
        if (failed(parser.parseTypeList(types)))
          return make_tuple(typeList, failure());
        for (auto &type : types)
          typeList.append(pybind11::cast(type));
        return make_tuple(typeList, success());
      })
      .def("parseRegionWithArguments", [](OpAsmParser &parser, Region &region)
                                       -> LogicalResult {
        SmallVector<OpAsmParser::OperandType, 4> args;
        SmallVector<Type, 4> tys;
        if (succeeded(parser.parseOptionalLParen())) {
          OpAsmParser::OperandType arg;
          Type ty;
          auto result = parser.parseOptionalOperand(arg);
          if (result.hasValue()) {
            if (*result || parser.parseColonType(ty))
              return failure();
            args.push_back(arg);
            tys.push_back(ty);
            while (succeeded(parser.parseOptionalComma())) {
              if (parser.parseOperand(arg) || parser.parseColonType(ty))
                return failure();
              args.push_back(arg);
              tys.push_back(ty);
            }
          }
          if (parser.parseRParen())
            return failure();
        }
        return parser.parseRegion(region, args, tys);
      });

  exposeAllLiteralParsers(parserCls);

  /// Utility types for parsing.
  class_<SMLoc>(m, "SMLoc");
  class_<OpAsmParser::OperandType>(m, "OperandType");
  class_<LogicalResult> logicalResultCls{m, "LogicalResult"};
  logicalResultCls.def("__bool__", &mlir::succeeded);
  class_<ParseResult>(m, "ParseResult", logicalResultCls);

  dmc::py::exposeOperationWrap(m);
  dmc::py::exposeResultWrap(m);
  dmc::py::exposeDynamicTypes(m);
}

} // end namespace py
} // end namespace mlir
