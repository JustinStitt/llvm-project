#include "llvm/Transforms/Utils/IdiomExclusions.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/IR/Analysis.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/PatternMatch.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;
using namespace llvm::PatternMatch;

namespace {

enum class OverflowIdiomTy {
  BasePlusOffsetCompareToBase, /*    if(a + b < a) {...} */
  WhileDec,                    /*    while(i--)    {...} */
};

/*struct OverflowIdiomInfo {*/
/*  OverflowIdiomTy Ty;*/
/*  BasicBlock *Overflow;*/
/*  BasicBlock *Cont;*/
/*};*/

/*bool isAddOverflowIntrinsic(const WithOverflowInst *WOI) {*/
/*  return WOI->getIntrinsicID() == Intrinsic::sadd_with_overflow ||*/
/*         WOI->getIntrinsicID() == Intrinsic::uadd_with_overflow;*/
/*}*/

/*bool doesMatchBasePlusOffsetCompareToBaseIdiom(BasicBlock *BB,*/
/*                                               ExtractValueInst *Sum,*/
/*                                               WithOverflowInst *WOI) {*/
/*  if (!BB || !Sum || !WOI)*/
/*    return false;*/
/**/
/*  if (!isAddOverflowIntrinsic(WOI))*/
/*    return false;*/
/**/
/*  if (!Sum->isUsedInBasicBlock(BB))*/
/*    return false;*/
/**/
/*  for (const User *SU : Sum->users()) {*/
/*    if (!isa<ICmpInst>(SU))*/
/*      return false;*/
/**/
/*    // Given that the comparison resembles some permutation of a + b < a,*/
/*    // OtherValue represents the non-addition side of the comparison*/
/*    Value *OtherValue;*/
/**/
/*    for (unsigned Idx = 0; Idx < SU->getNumOperands(); ++Idx)*/
/*      if (SU->getOperand(Idx) != Sum)*/
/*        OtherValue = SU->getOperand(Idx);*/
/**/
/*    // Determine if OtherValue was a part of the Sum to begin with...*/
/*    // Easy, OtherValue is equal to one of the addends*/
/*    if (is_contained(WOI->args(), OtherValue))*/
/*      continue;*/
/**/
/*    // Harder... OtherValue may be a 'load' or 'phi' instruction*/
/*    if (const PHINode *Phi = dyn_cast<PHINode>(OtherValue)) {*/
/*      if (is_contained(WOI->args(),*/
/*                       Phi->getIncomingValueForBlock(WOI->getParent()))) {*/
/*        continue;*/
/*      }*/
/*    } else if (const LoadInst *OtherL = dyn_cast<LoadInst>(OtherValue)) {*/
/*      // Ensure the pointer references one of the addends from the sum*/
/*      auto PointerOperandMatches = [&](const Use &U) {*/
/*        const LoadInst *L = dyn_cast<LoadInst>(&U);*/
/*        return L && OtherL->getPointerOperand() == L->getPointerOperand();*/
/*      };*/
/**/
/*      if (any_of(WOI->args(), PointerOperandMatches))*/
/*        continue;*/
/*    }*/
/*    return false;*/
/*  }*/
/**/
/*  return true;*/
/*}*/

/*SmallVector<OverflowIdiomInfo> findOverflowIdioms(Function &F) {*/
/**/
/*  SmallVector<OverflowIdiomInfo> Infos;*/
/**/
/*  for (Instruction &I : instructions(F)) {*/
/*    WithOverflowInst *WOI = dyn_cast<WithOverflowInst>(&I);*/
/*    if (!WOI)*/
/*      continue;*/
/**/
/*    ExtractValueInst *Result = nullptr;*/
/*    ExtractValueInst *Overflow = nullptr;*/
/**/
/*    for (User *U : WOI->users()) {*/
/*      if (auto *EVI = dyn_cast<ExtractValueInst>(U)) {*/
/*        assert(EVI->getNumIndices() == 1 &&*/
/*               "This aggregate should only have 1 index");*/
/*        if (EVI->getIndices()[0] == 0)*/
/*          Result = EVI;*/
/*        else if (EVI->getIndices()[0] == 1) {*/
/*          Overflow = EVI;*/
/*        }*/
/*      }*/
/*    }*/
/**/
/*    if (!Overflow || Overflow->users().empty())*/
/*      continue;*/
/**/
/*    Instruction *Br = WOI->getParent()->getTerminator();*/
/*    assert(Br && Br->getNumSuccessors() == 2 &&*/
/*           "A BasicBlock with an overflow intrinsic should have a terminator "*/
/*           "with two successors");*/
/**/
/*    // Assume true branch is Cont and false branch is OverflowHandler *///
/*    BasicBlock *Cont = Br->getSuccessor(0);*/
/*    BasicBlock *OverflowHandler = Br->getSuccessor(1);*/
/**/
/*    // TODO: what if Cont only has 1 predecessor too, does this happen when the*/
/*    // handler block is trapping?*/
/*    // The overflow-handling block will only have one predecessor,//
//     * change our assumption from above if we got it wrong ////
//    if (Br->getSuccessor(0)->hasNPredecessors(1)) {*/
/*      OverflowHandler = Br->getSuccessor(0);*/
/*      Cont = Br->getSuccessor(1);*/
/*    }*/
/**/
/*    if (doesMatchBasePlusOffsetCompareToBaseIdiom(Cont, Result, WOI)) {*/
/*      OverflowIdiomInfo Info{*/
/*          OverflowIdiomTy::BasePlusOffsetCompareToBase,*/
/*          WOI,*/
/*          OverflowHandler,*/
/*          Cont,*/
/*      };*/
/*      Infos.push_back(std::move(Info));*/
/*    }*/
/*  }*/
/**/
/*  return Infos;*/
/*}*/

/// Remove the edge that connects the BasicBlock containing the overflow
/// intrinsic to the overflow-handling BasicBlock. Now there is a single edge
/// to the non-overflow handling BasicBlock.
void removeEdgeToOverflowHandler(Instruction *I) {
  if (!I)
    return;
  BasicBlock *Cont = I->getParent();
  BasicBlock *Entry = nullptr;
  BasicBlock *Overflow = nullptr;

  for (const BasicBlock *BB : predecessors(I->getParent())) {
    const Instruction *Br = BB->getTerminator();
    assert(Br && "Malformed basic block has no terminator");
    if (Br->getNumSuccessors() == 1)
      Overflow = const_cast<BasicBlock *>(BB);
    else if (Br->getNumSuccessors() == 2)
      Entry = const_cast<BasicBlock *>(BB);
    else
      llvm_unreachable(
          "Must be 1 or 2 successors in overflow handling pattern");
  }

  assert(Entry && Cont && Overflow);
  /*errs() << "Wanting to change/remove: "; Cont->getTerminator()->dump();*/
  BranchInst::Create(Cont, Entry->getTerminator());
  Cont->removePredecessor(Overflow,
                          /*KeepOneInputPHIs=*/false);
  Entry->getTerminator()->eraseFromParent();
}

} // namespace

SmallVector<Instruction *> NewMatching(Function &F) {
  SmallVector<Instruction *> MatchingInstructions;
  const DataLayout &DL = F.getParent()->getDataLayout();

  for (Instruction &I : instructions(F)) {
    Value *CmpOther;
    Value *LHS, *RHS;
    Value *SomeLoad;
    ICmpInst::Predicate Pred;

    if (match(&I, m_c_ICmp(Pred,
                           m_ExtractValue<0>(
                               m_Intrinsic<Intrinsic::uadd_with_overflow>(
                                   m_Value(LHS), m_Value(RHS))),
                           m_Value(CmpOther)))) {
      errs() << "strip dump LHS: "; LHS->stripInBoundsOffsets()->dump();
      errs() << "strip dump RHS: "; RHS->stripInBoundsOffsets()->dump();
      errs() << "strip dump CmpOther: "; CmpOther->stripInBoundsOffsets()->dump();
      /*CmpOther->getUnderlyingObject()*/
      errs() << "underlying CmpOther: "; getUnderlyingObject(CmpOther)->dump();

      if (CmpOther == LHS || CmpOther == RHS) {
        errs() << "Matched first cond\n";
        MatchingInstructions.push_back(&I);
        continue;
      }
      LoadInst *RHSLoad = dyn_cast<LoadInst>(RHS);
      LoadInst *LHSLoad = dyn_cast<LoadInst>(LHS);
      auto IsValueOneOfLHSorRHSPtrOp = [&](Value *L) {
        // check GEP
        errs() << "L: "; L->dump();
        if (GetElementPtrInst *GEP = dyn_cast<GetElementPtrInst>(L)) {
          auto ByteOffset =
              GEP->getPointerOffsetFrom(getUnderlyingObject(GEP), DL);
          if (GetElementPtrInst *LHSGEP = dyn_cast<GetElementPtrInst>(LHSLoad->getPointerOperand())) {
            if ((getUnderlyingObject(LHSGEP) == getUnderlyingObject(GEP)) && (ByteOffset == LHSGEP->getPointerOffsetFrom(getUnderlyingObject(LHSGEP), DL))) {
              errs() << "LHSGEP match\n";
              return true;
            }
          }
          if (GetElementPtrInst *RHSGEP = dyn_cast<GetElementPtrInst>(RHSLoad->getPointerOperand())) {
            if ((getUnderlyingObject(RHSGEP) == getUnderlyingObject(GEP)) && (ByteOffset == RHSGEP->getPointerOffsetFrom(getUnderlyingObject(RHSGEP), DL))) {
              errs() << "RHSGEP match\n";
              return true;
            }
          }
          /*errs() << "GEP: "; GEP->dump();*/
          /*errs() << "LHSLoad: "; LHSLoad->dump();*/
          /*errs() << "LHSLoad underlying: "; getUnderlyingObject(LHSLoad)->dump();*/
          /*errs() << "RHSLoad: "; RHSLoad->dump();*/
          /*errs() << "ByteOffset: " << ByteOffset << "\n";*/
          return getUnderlyingObject(GEP) == getUnderlyingObject(LHSLoad->getPointerOperand());
        }

        return (LHSLoad && L == LHSLoad->getPointerOperand()) ||
               (RHSLoad && L == RHSLoad->getPointerOperand());
      };
      if (match(CmpOther, m_Load(m_Value(SomeLoad)))) {
        LoadInst *LL = dyn_cast<LoadInst>(CmpOther);
        /*errs() << "no-strip dump CmpOther2: "; LL->getPointerOperand()->dump();*/
        /*errs() << "strip dump CmpOther2: "; LL->getPointerOperand()->stripInBoundsOffsets()->dump();*/
        errs() << "underlying CmpOther 2: "; getUnderlyingObject(LL->getPointerOperand())->dump();
        errs() << "underlying LHS: "; getUnderlyingObject(LHS->stripPointerCasts())->dump();
        errs() << "underlying RHS: "; getUnderlyingObject(RHS)->dump();
        if (IsValueOneOfLHSorRHSPtrOp(SomeLoad)) {
          errs() << "Matched second cond\n";
          MatchingInstructions.push_back(&I);
          continue;
        }

      }
      if (PHINode *Phi = dyn_cast<PHINode>(CmpOther)) {
        if (Phi->getNumIncomingValues() != 2)
          continue; // no match
        LoadInst *Phi0 = dyn_cast<LoadInst>(Phi->getIncomingValue(0));
        LoadInst *Phi1 = dyn_cast<LoadInst>(Phi->getIncomingValue(1));
        if (IsValueOneOfLHSorRHSPtrOp(Phi0->getPointerOperand()) ||
            IsValueOneOfLHSorRHSPtrOp(Phi1->getPointerOperand())) {
          errs() << "Phi match\n";
          MatchingInstructions.push_back(&I);
          continue;
        }
      }
    }
  }

  return MatchingInstructions;
}

PreservedAnalyses IdiomExclusionsPass::run(Function &F,
                                           FunctionAnalysisManager &AM) {
  errs() << "Running IdiomExclusionsPass\n";
  SmallVector<Instruction *> MatchingInstructions = NewMatching(F);
  for (Instruction *I : MatchingInstructions)
    removeEdgeToOverflowHandler(I);

  return PreservedAnalyses::none();
}
