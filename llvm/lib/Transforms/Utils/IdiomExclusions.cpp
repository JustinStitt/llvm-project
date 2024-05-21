#include "llvm/Transforms/Utils/IdiomExclusions.h"
#include "llvm/IR/Analysis.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/PassManager.h"

using namespace llvm;

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



  const Value *SumValue = nullptr;
  const Value *OverflowValue = nullptr;

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

    switch (Indices[EVI->getAggregateOperandIndex()]) {
      case 0:
        SumValue = IntrinsicUse.getUser();
        break;
      case 1:
        OverflowValue = IntrinsicUse.getUser();
        break;
      default:
        errs() << "bad index in extractvalue!\n";
        return PreservedAnalyses::all();
    }

  }

  if (!SumValue || !OverflowValue) {
    errs() << "Didn't find one of SumValue or OverflowValue!\n";
    return PreservedAnalyses::all();
  }

  errs() << "SumValue: "; SumValue->dump();
  errs() << "OverflowValue: "; OverflowValue->dump();

  const BasicBlock *BBSucc = nullptr;

  for (const User *OverflowUser : OverflowValue->users()) {
    if (const BranchInst *BI = dyn_cast<BranchInst>(OverflowUser)) {
      if (BI->getNumSuccessors() != 2) continue;
      BBSucc = BI->getSuccessor(1); // If no overflow
    }
  }

  if (!BBSucc) {
    errs() << "Didn't find BasicBlock successor for overflow case!\n";
    return PreservedAnalyses::all();
  }

  errs() << "BBSucc->dump(): "; BBSucc->dump();

  const PHINode *Phi = dyn_cast<PHINode>(BBSucc->getIterator()->begin());

  if (!Phi) {
    errs() << "Failed to convert first instruction of BBSucc to Phi node!\n";
    return PreservedAnalyses::all();
  }

  if(const ICmpInst *Comp = dyn_cast<ICmpInst>(BBSucc->getFirstNonPHI())) {
    /* Signed Less Than or Unsigned Less Than */
    if (Comp->getSignedPredicate() == ICmpInst::Predicate::ICMP_SLT ||
        Comp->getSignedPredicate() == ICmpInst::Predicate::ICMP_ULT) {
      bool LHSMatchesSum = Comp->getOperand(0) == SumValue;
      bool RHSMatchesPhi = Comp->getOperand(1) == Phi;
      errs() << "Same Operand{0}: " << LHSMatchesSum << "\n";
      errs() << "Same Operand{1}: " << RHSMatchesPhi << "\n";
      if (LHSMatchesSum && RHSMatchesPhi) {
        errs() << "Found IDIOM EXCLUSION matching if (a + b < a)\n";
      }
    }
  }

  return PreservedAnalyses::all();
}

