#pragma once

#include "Kinds.h"
#include "dmc/Dynamic/DynamicOperation.h"
#include "dmc/Spec/SpecTypeImplementation.h"
#include "dmc/Spec/SpecAttrImplementation.h"
#include "dmc/Spec/SpecRegion.h"
#include "dmc/Spec/SpecSuccessor.h"
#include "dmc/Spec/OpType.h"
#include "dmc/Spec/NamedConstraints.h"

#include <mlir/IR/Attributes.h>
#include <mlir/IR/OpDefinition.h>

namespace dmc {

/// For operands and results, more than one variadic value requires a size
/// specification trait. Only one of SameVariadicSizes or SizedSegments may
/// be used.
///
/// These traits mark an Operation with variadic operands or results with a
/// size specification that all variadic values have the same array size.
struct SameVariadicOperandSizes : public DynamicTrait {
  static llvm::StringRef getName() { return "SameVariadicOperandSizes"; }

  /// Verify variadic operand formation.
  mlir::LogicalResult verifyOp(mlir::Operation *op) const override;

  /// Variadic operand group getter. Analogous to getODSOperands().
  static mlir::ValueRange getGroup(mlir::Operation *op, unsigned idx);
};
struct SameVariadicResultSizes : public DynamicTrait {
  static llvm::StringRef getName() { return "SameVariadicResultSizes"; }

  /// Verify variadic result formation.
  mlir::LogicalResult verifyOp(mlir::Operation *op) const override;

  /// Variadic result group getter. Analogous to getODSResults().
  static mlir::ValueRange getGroup(mlir::Operation *op, unsigned idx);
};

/// These traits indicate that the Operation has variadic operands or result
/// with sizes known at runtime, captured inside an attribute.
struct SizedOperandSegments : public DynamicTrait {
  static llvm::StringRef getName() { return "SizedOperandSegments"; }

  /// Based off mlir::AttrSizedOperandSegments.
  using Base = mlir::OpTrait::AttrSizedOperandSegments<DynamicTrait>;

  /// Verify the operand segment size attribute.
  mlir::LogicalResult verifyOp(mlir::Operation *op) const override;

  /// Get the operand segment size attribute.
  static mlir::DenseIntElementsAttr getSegmentSizesAttr(mlir::Operation *op);

  /// Variadic operand group getter. Analogous to getODSOperands().
  static mlir::ValueRange getGroup(mlir::Operation *op, unsigned idx);
};
struct SizedResultSegments : public DynamicTrait {
  static llvm::StringRef getName() { return "SizedResultSegments"; }

  /// Based off mlir::AttrSizedResultSegments.
  using Base = mlir::OpTrait::AttrSizedResultSegments<DynamicTrait>;

  /// Verify the result segment size attribute.
  mlir::LogicalResult verifyOp(mlir::Operation *op) const override;

  /// Get the result segment size attribute.
  static mlir::DenseIntElementsAttr getSegmentSizesAttr(mlir::Operation *op);

  /// Variadic result group getter. Analogous to getODSResults().
  static mlir::ValueRange getGroup(mlir::Operation *op, unsigned idx);
};

/// Top-level Type and Attribute verifiers apply the specified constraints
/// on an Operation. Trait validity and interactions are already verified on
/// the OperationOp spec.
class TypeConstraintTrait : public DynamicTrait {
public:
  static llvm::StringRef getName() { return "TypeConstraintTrait"; }

  /// Create a type constraint with types wrapped in a FunctionType.
  inline explicit TypeConstraintTrait(OpType opTy)
      : opTy{opTy} {}

  /// Check the Op's operand and result types.
  inline mlir::LogicalResult verifyOp(mlir::Operation *op) const override {
    return impl::verifyTypeConstraints(op, opTy);
  }

  inline auto getOpType() { return opTy; }

private:
  OpType opTy;
};

class AttrConstraintTrait : public DynamicTrait {
public:
  static llvm::StringRef getName() { return "AttrConstraintTrait"; }

  /// Create an attribute constraint with attributes
  /// wrapped in a DictionaryAttr.
  inline explicit AttrConstraintTrait(mlir::DictionaryAttr opAttrs)
      : opAttrs{opAttrs} {}

  /// Check the Op's attributes.
  inline mlir::LogicalResult verifyOp(mlir::Operation *op) const override {
    return impl::verifyAttrConstraints(op, opAttrs);
  }

  inline auto getOpAttrs() { return opAttrs; }

private:
  mlir::DictionaryAttr opAttrs;
};

/// Top-level Region verifier applies the given constraints on an Operation's
/// regions. Validity of the constraints is verified on the Operation spec.
class RegionConstraintTrait : public DynamicTrait {
public:
  static llvm::StringRef getName() { return "RegionConstraintTrait"; }

  /// Create a region constraint with constraints in an ArrayAttr.
  inline explicit RegionConstraintTrait(OpRegion opRegions)
      : opRegions{opRegions} {}

  /// Check the Op's regions.
  inline mlir::LogicalResult verifyOp(mlir::Operation *op) const override {
    return impl::verifyRegionConstraints(op, opRegions);
  }

  inline auto getOpRegions() { return opRegions; }

private:
  OpRegion opRegions;
};

/// Top-level successor verifier applies the given constraints on an Operation's
/// successors. Validity of the constraints is verified on the Operation spec.
class SuccessorConstraintTrait : public DynamicTrait {
public:
  static llvm::StringRef getName() { return "SuccessorConstraintTrait"; }

  /// Create a successor constraint with constraints in an ArrayAttr.
  inline explicit SuccessorConstraintTrait(OpSuccessor opSuccs)
      : opSuccs{opSuccs} {}

  /// Check the Op's successors.
  inline mlir::LogicalResult verifyOp(mlir::Operation *op) const override {
    return impl::verifySuccessorConstraints(op, opSuccs);
  }

  inline auto getOpSuccessors() { return opSuccs; }

private:
  OpSuccessor opSuccs;
};

} // end namespace dmc
