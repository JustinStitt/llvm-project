#ifndef LLVM_TRANSFORMS_UTILS_IDIOMEXCL_H
#define LLVM_TRANSFORMS_UTILS_IDIOMEXCL_H

#include "llvm/IR/Analysis.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/PassManager.h"

namespace llvm {

class IdiomExclusionsPass : public PassInfoMixin<IdiomExclusionsPass> {
private:
  SmallVector<WithOverflowInst *>
  checkOverflowIntstructions(Function &F, FunctionAnalysisManager &AM);

public:
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);
  static bool isRequired() { return true; }
};

} // namespace llvm

#endif // LLVM_TRANSFORMS_UTILS_IDIOMEXCL_H
