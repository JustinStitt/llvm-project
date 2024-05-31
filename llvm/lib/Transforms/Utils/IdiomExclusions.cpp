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

enum class OverflowIdiomTy {
  BasePlusOffsetCompareToBase, /*    if(a + b < a) {...} */
  WhileDec,                    /*    while(i--)    {...} */
};

struct OverflowIdiomInfo {
  OverflowIdiomTy Ty;
  WithOverflowInst *WOI;
  BasicBlock *OverflowHandler;
  BasicBlock *Cont;
};

bool isAddOverflowIntrinsic(const WithOverflowInst *WOI) {
  return WOI->getIntrinsicID() == Intrinsic::sadd_with_overflow ||
         WOI->getIntrinsicID() == Intrinsic::uadd_with_overflow;
}

bool doesMatchBasePlusOffsetCompareToBaseIdiom(BasicBlock *BB,
                                               ExtractValueInst *Sum,
                                               WithOverflowInst *WOI) {
  if (!BB || !Sum || !WOI)
    return false;

  if (!isAddOverflowIntrinsic(WOI))
    return false;

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

SmallVector<OverflowIdiomInfo> findOverflowIdioms(Function &F) {

  SmallVector<WithOverflowInst *> OverflowInstrinsics;
  SmallVector<OverflowIdiomInfo> Infos;

  for (Instruction &Inst : instructions(F)) {
    WithOverflowInst *WOI = dyn_cast<WithOverflowInst>(&Inst);
    if (WOI) {
      OverflowInstrinsics.push_back(WOI);
    }
  }

  if (OverflowInstrinsics.empty())
    return Infos;

  for (WithOverflowInst *WOI : OverflowInstrinsics) {
    ExtractValueInst *Result = nullptr;
    ExtractValueInst *Overflow = nullptr;

    for (User *U : WOI->users()) {
      if (auto *EVI = dyn_cast<ExtractValueInst>(U)) {
        assert(EVI->getNumIndices() == 1 &&
               "This aggregate should only have 1 index");
        if (EVI->getIndices()[0] == 0)
          Result = EVI;
        else if (EVI->getIndices()[0] == 1) {
          Overflow = EVI;
        }
      }
    }

    if (Overflow->users().empty())
      continue;

    Instruction *Terminator = WOI->getParent()->getTerminator();
    assert(Terminator && "Malformed BasicBlock containing Overflow intrinsic");

    BranchInst *Br = dyn_cast<BranchInst>(Terminator);

    assert(Br->getNumSuccessors() == 2 &&
           "A BasicBlock with an overflow intrinsic should have a terminator "
           "with two successors");

    /* Assume true branch is Cont and false branch is OverflowHandler */
    BasicBlock *Cont = Br->getSuccessor(0);
    BasicBlock *OverflowHandler = Br->getSuccessor(1);

    /* The overflow-handling block will only have one predecessor,
     * change our assumption from above if we got it wrong */
    if (Br->getSuccessor(0)->hasNPredecessors(1)) {
      OverflowHandler = Br->getSuccessor(0);
      Cont = Br->getSuccessor(1);
    }

    if (doesMatchBasePlusOffsetCompareToBaseIdiom(Cont, Result, WOI)) {
      OverflowIdiomInfo Info{
          OverflowIdiomTy::BasePlusOffsetCompareToBase,
          WOI,
          OverflowHandler,
          Cont,
      };
      Infos.push_back(std::move(Info));
    }
  }

  return Infos;
}

/// Remove the edge that connects the BasicBlock containing the overflow
/// intrinsic to the overflow-handling BasicBlock. Now there is a single edge
/// to the non-overflow handling BasicBlock.
void removeEdgeToOverflowHandler(OverflowIdiomInfo &Info) {
  Instruction *Terminator = Info.WOI->getParent()->getTerminator();
  assert(Terminator && "Malformed BasicBlock containing Overflow intrinsic");
  BranchInst *Br = dyn_cast<BranchInst>(Terminator);
  BranchInst::Create(Info.Cont, Br);
  Info.Cont->removePredecessor(Info.OverflowHandler,
                               /*KeepOneInputPHIs=*/false);
  Br->eraseFromParent();
}

} // namespace

PreservedAnalyses IdiomExclusionsPass::run(Function &F,
                                           FunctionAnalysisManager &AM) {
  SmallVector<OverflowIdiomInfo> Infos = findOverflowIdioms(F);

  if (Infos.empty())
    return PreservedAnalyses::all();

  for (OverflowIdiomInfo &Info : Infos)
    removeEdgeToOverflowHandler(Info);

  return PreservedAnalyses::none();
}
