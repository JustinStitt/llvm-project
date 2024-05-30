#include "llvm/Transforms/Utils/IdiomExclusions.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/IR/Analysis.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/PassManager.h"

using namespace llvm;

namespace {
bool isAddOverflowIntrinsic(const WithOverflowInst *WOI) {
  if (!WOI)
    return false;

  return WOI->getIntrinsicID() == Intrinsic::sadd_with_overflow ||
         WOI->getIntrinsicID() == Intrinsic::uadd_with_overflow;
}

bool isAPlusBCMPAOrB(BasicBlock *BB, ExtractValueInst *Sum,
                     WithOverflowInst *WOI) {
  assert(BB);
  assert(Sum);
  assert(WOI);

  if (!Sum->isUsedInBasicBlock(BB))
    return false;

  for (const User *SU : Sum->users()) {
    if (!isa<ICmpInst>(SU))
      return false;

    // Given that the comparison resembles some permutation of a + b < a,
    // OtherValue represents the non-addition side of the comparison
    Value *OtherValue;

    for (unsigned Idx = 0; Idx < SU->getNumOperands(); ++Idx)
      if (SU->getOperand(Idx) != Sum)
        OtherValue = SU->getOperand(Idx);

    // Determine if OtherValue was a part of the Sum to begin with...
    // Easy, OtherValue is equal to one of the addends
    if (is_contained(WOI->args(), OtherValue))
      continue;

    // Harder... OtherValue may be a 'load' or 'phi' instruction
    if (const PHINode *Phi = dyn_cast<PHINode>(OtherValue)) {
      if (is_contained(WOI->args(),
                       Phi->getIncomingValueForBlock(WOI->getParent()))) {
        continue;
      }
    } else if (const LoadInst *OtherL = dyn_cast<LoadInst>(OtherValue)) {
      // Ensure the pointer references one of the addends from the sum
      auto PointerOperandMatches = [&](const Use &U) {
        const LoadInst *L = dyn_cast<LoadInst>(&U);
        return L && OtherL->getPointerOperand() == L->getPointerOperand();
      };

      if (any_of(WOI->args(), PointerOperandMatches))
        continue;
    }
    return false;
  }

  return true;
}

} // namespace

/* Return the Overflow intrinsics that can be removed */
SmallVector<WithOverflowInst *>
IdiomExclusionsPass::checkOverflowIntstructions(Function &F,
                                                FunctionAnalysisManager &AM) {
  // TODO: Expand to sub for stuff like "while (x--)"
  SmallVector<WithOverflowInst *> AddOverflowInstrinsics;
  SmallVector<WithOverflowInst *> RemovableOverflowIntrinsics;

  // Locate the CallInstr's, looking specifically for add overflow intrinsics
  for (Instruction &Inst : instructions(F)) {
    WithOverflowInst *WOI = dyn_cast<WithOverflowInst>(&Inst);
    if (WOI && isAddOverflowIntrinsic(WOI)) {
      AddOverflowInstrinsics.push_back(WOI);
    }
  }

  if (AddOverflowInstrinsics.empty())
    return RemovableOverflowIntrinsics;

  errs() << "AddOverflowInstrinsics.size(): " << AddOverflowInstrinsics.size()
         << "\n";

  for (WithOverflowInst *WOI : AddOverflowInstrinsics) {
    ExtractValueInst *Sum = nullptr;
    ExtractValueInst *Overflow = nullptr;

    for (User *U : WOI->users()) {
      if (auto *EVI = dyn_cast<ExtractValueInst>(U)) {
        assert(EVI->getNumIndices() == 1 &&
               "This aggregate should only have 1 index");
        if (EVI->getIndices()[0] == 0)
          Sum = EVI;
        else if (EVI->getIndices()[0] == 1) {
          Overflow = EVI;
        }
      }
    }

    Instruction *Terminator = WOI->getParent()->getTerminator();
    assert(Terminator && "Malformed BasicBlock containing Overflow intrinsic");

    BranchInst *Br = dyn_cast<BranchInst>(Terminator);

    assert(Br->getNumSuccessors() == 2 &&
           "A BasicBlock with an overflow intrinsic should have a terminator "
           "with two successors");

    BasicBlock *NonOverflowBB = nullptr;
    BasicBlock *OverflowBB = nullptr;

    for (unsigned I = 0; I < Br->getNumSuccessors(); ++I) {
      BasicBlock *Succ = Br->getSuccessor(I);
      if (any_of(*Succ,
                 [](const Instruction &Inst) { return isa<CallInst>(Inst); })) {
        OverflowBB = Succ;
      } else {
        NonOverflowBB = Succ;
      }
    }

    assert(NonOverflowBB && OverflowBB &&
           "Both of these BasicBlocks should be defined");

    // case like: if (a + b < a)
    if (isAPlusBCMPAOrB(NonOverflowBB, Sum, WOI)) {
      BranchInst::Create(NonOverflowBB, Br);
      NonOverflowBB->removePredecessor(OverflowBB,
                                       /*KeepOneInputPHIs=*/false);
      Br->eraseFromParent();
      RemovableOverflowIntrinsics.push_back(WOI);
    }

    assert(Sum && Overflow &&
           "These should've been set if WOI had valid users");
  }

  return RemovableOverflowIntrinsics;
}

PreservedAnalyses IdiomExclusionsPass::run(Function &F,
                                           FunctionAnalysisManager &AM) {
  SmallVector<WithOverflowInst *> Removable = checkOverflowIntstructions(F, AM);
  errs() << "Count of removable WOI's: " << Removable.size() << "\n";
  return PreservedAnalyses::none();
}
