#include "llvm/Transforms/Utils/IdiomExclusions.h"
#include "llvm/IR/Analysis.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/PassManager.h"

using namespace llvm;

namespace {
  enum class OverflowExtractKind {
    Sum, Overflow
  };
} // namespace

static bool IsSignedAddOverflowIntrinsic(const IntrinsicInst *II) {
  return II->getIntrinsicID() == Intrinsic::sadd_with_overflow;
}

PreservedAnalyses IdiomExclusionsPass::run(Function &F,
                                           FunctionAnalysisManager &AM) {
  errs() << "F: " << F.getName() << "\n";

  const IntrinsicInst *AddIntrinsic = nullptr;

  std::vector<BasicBlock *> BasicBlocks;
  for (BasicBlock &BB : F) {
    /* const Module *Module = BB.getModule(); */
    errs() << "\\--BB: " << BB.getName() << "\n";
    for (const Instruction &Inst : BB) {
      Inst.dump();
      /* if (AddIntrinsic) { */
      /*   errs() << "Found AddIntrinsic!\n"; */
      /* } */
      if (auto *II = dyn_cast<IntrinsicInst>(&Inst)) {
        if (IsSignedAddOverflowIntrinsic(II)) {
          AddIntrinsic = II;
          break;
        }
        /* errs() << "^^^ calling: " << II->getCalledOperand()->getName() << "\t" */
        /*   << II->getIntrinsicID() << "\t" << Intrinsic::sadd_with_overflow << */
        /*   "\t" << II->getNameOrAsOperand() << "\t" <<  II->getName() << "\n"; */
      }
    }
  }

  // this seems to get the operands that feed into the add intrinsic
  if (!AddIntrinsic) {
    errs() << "No AddIntrinsic found!\n";
    return PreservedAnalyses::all();
  }
  errs() << "End BB\n";
  errs() << "NumOperands: " << AddIntrinsic->getNumOperands() << "\n";
  /* const Use *IntrinsicUses = AddIntrinsic->getOperandList(); */
  for (unsigned i = 0; i < AddIntrinsic->getNumOperands(); ++i) {
    const Value *InstrinsicOperandValue = AddIntrinsic->getOperandList()[i];
    /* const Use *IntrinsicUse = &IntrinsicUses[i]; */
    /* const Value *IntrinsicUseValue = IntrinsicUse->get(); */
    errs() << "IntrinsicUseValue->dump(): ";
    InstrinsicOperandValue->dump();
  }

  // check where the AddIntrinsic value (result) is then used
  /* const Use *AddIntrinsicUse = dyn_cast<Use>(AddIntrinsic); */
  /* const Use &AddIntrinsicUse = AddIntrinsic->getOperandUse(); */
  for (const Use &IntrinsicUse : AddIntrinsic->uses()) {

    /* const Instruction *Instr = dyn_cast<Instruction>(IntrinsicUse.getUser()); */
    /* errs() << "Instr: "; */
    /* Instr->dump(); */

    const ExtractValueInst *EVI =
      dyn_cast<ExtractValueInst>(IntrinsicUse.getUser()); 

    EVI->dump();

    if (!EVI) {
      errs() << "Using Overflow Intrinsic result in non-extractvalue instruction!\n";
      return PreservedAnalyses::all();
    }

    ArrayRef<unsigned> Indices = EVI->getIndices();

    if (Indices.empty()) {
      errs() << "No indices are present on extractvalue instruction!\n";
      return PreservedAnalyses::all();
    }

    OverflowExtractKind Kind;
    switch (Indices[EVI->getAggregateOperandIndex()]) {
      case 0:
        Kind = OverflowExtractKind::Sum;
        break;
      case 1:
        Kind = OverflowExtractKind::Overflow;
        break;
      default:
        errs() << "bad index in extractvalue!\n";
        return PreservedAnalyses::all();
    }

    errs() << "OverflowExtractKind: " << (unsigned)Kind << "\n";

  }

  return PreservedAnalyses::all();
}
