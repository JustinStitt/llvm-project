#ifndef LLVM_TRANSFORMS_UTILS_IDIOMEXCL_H
#define LLVM_TRANSFORMS_UTILS_IDIOMEXCL_H

#include "llvm/IR/Analysis.h"
#include "llvm/IR/PassManager.h"

namespace llvm {

class IdiomExclusionsPass : public PassInfoMixin<IdiomExclusionsPass> {
public:
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);
};

} // namespace llvm

#endif // LLVM_TRANSFORMS_UTILS_IDIOMEXCL_H
