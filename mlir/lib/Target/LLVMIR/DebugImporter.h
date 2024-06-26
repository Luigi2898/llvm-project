//===- DebugImporter.h - LLVM to MLIR Debug conversion -------*- C++ -*----===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements the translation between LLVMIR debug information and
// the corresponding MLIR representation.
//
//===----------------------------------------------------------------------===//

#ifndef MLIR_LIB_TARGET_LLVMIR_DEBUGIMPORTER_H_
#define MLIR_LIB_TARGET_LLVMIR_DEBUGIMPORTER_H_

#include "mlir/Dialect/LLVMIR/LLVMDialect.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/MLIRContext.h"
#include "llvm/IR/DebugInfoMetadata.h"

namespace mlir {
class Operation;

namespace LLVM {
class LLVMFuncOp;

namespace detail {

class DebugImporter {
public:
  DebugImporter(ModuleOp mlirModule)
      : context(mlirModule.getContext()), mlirModule(mlirModule) {}

  /// Translates the given LLVM debug location to an MLIR location.
  Location translateLoc(llvm::DILocation *loc);

  /// Translates the LLVM DWARF expression metadata to MLIR.
  DIExpressionAttr translateExpression(llvm::DIExpression *node);

  /// Translates the LLVM DWARF global variable expression metadata to MLIR.
  DIGlobalVariableExpressionAttr
  translateGlobalVariableExpression(llvm::DIGlobalVariableExpression *node);

  /// Translates the debug information for the given function into a Location.
  /// Returns UnknownLoc if `func` has no debug information attached to it.
  Location translateFuncLocation(llvm::Function *func);

  /// Translates the given LLVM debug metadata to MLIR.
  DINodeAttr translate(llvm::DINode *node);

  /// Infers the metadata type and translates it to MLIR.
  template <typename DINodeT>
  auto translate(DINodeT *node) {
    // Infer the MLIR type from the LLVM metadata type.
    using MLIRTypeT = decltype(translateImpl(node));
    return cast_or_null<MLIRTypeT>(
        translate(static_cast<llvm::DINode *>(node)));
  }

private:
  /// Translates the given LLVM debug metadata to the corresponding attribute.
  DIBasicTypeAttr translateImpl(llvm::DIBasicType *node);
  DICompileUnitAttr translateImpl(llvm::DICompileUnit *node);
  DICompositeTypeAttr translateImpl(llvm::DICompositeType *node);
  DIDerivedTypeAttr translateImpl(llvm::DIDerivedType *node);
  DIFileAttr translateImpl(llvm::DIFile *node);
  DILabelAttr translateImpl(llvm::DILabel *node);
  DILexicalBlockAttr translateImpl(llvm::DILexicalBlock *node);
  DILexicalBlockFileAttr translateImpl(llvm::DILexicalBlockFile *node);
  DIGlobalVariableAttr translateImpl(llvm::DIGlobalVariable *node);
  DILocalVariableAttr translateImpl(llvm::DILocalVariable *node);
  DIModuleAttr translateImpl(llvm::DIModule *node);
  DINamespaceAttr translateImpl(llvm::DINamespace *node);
  DIScopeAttr translateImpl(llvm::DIScope *node);
  DISubprogramAttr translateImpl(llvm::DISubprogram *node);
  DISubrangeAttr translateImpl(llvm::DISubrange *node);
  DISubroutineTypeAttr translateImpl(llvm::DISubroutineType *node);
  DITypeAttr translateImpl(llvm::DIType *node);

  /// Constructs a StringAttr from the MDString if it is non-null. Returns a
  /// null attribute otherwise.
  StringAttr getStringAttrOrNull(llvm::MDString *stringNode);

  /// Get the DistinctAttr used to represent `node` if one was already created
  /// for it, or create a new one if not.
  DistinctAttr getOrCreateDistinctID(llvm::DINode *node);

  /// Get the `getRecSelf` constructor for the translated type of `node` if its
  /// translated DITypeAttr supports recursion. Otherwise, returns nullptr.
  function_ref<DIRecursiveTypeAttrInterface(DistinctAttr)>
  getRecSelfConstructor(llvm::DINode *node);

  /// A mapping between LLVM debug metadata and the corresponding attribute.
  DenseMap<llvm::DINode *, DINodeAttr> nodeToAttr;
  /// A mapping between distinct LLVM debug metadata nodes and the corresponding
  /// distinct id attribute.
  DenseMap<llvm::DINode *, DistinctAttr> nodeToDistinctAttr;

  /// A stack that stores the metadata nodes that are being traversed. The stack
  /// is used to detect cyclic dependencies during the metadata translation.
  /// A node is pushed with a null value. If it is ever seen twice, it is given
  /// a recursive id attribute, indicating that it is a recursive node.
  llvm::MapVector<llvm::DINode *, DistinctAttr> translationStack;
  /// All the unbound recursive self references in the translation stack.
  SmallVector<DenseSet<DistinctAttr>> unboundRecursiveSelfRefs;

  MLIRContext *context;
  ModuleOp mlirModule;
};

} // namespace detail
} // namespace LLVM
} // namespace mlir

#endif // MLIR_LIB_TARGET_LLVMIR_DEBUGIMPORTER_H_
