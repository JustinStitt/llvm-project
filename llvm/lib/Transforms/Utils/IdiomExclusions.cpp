#include "llvm/Transforms/Utils/IdiomExclusions.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/IR/Analysis.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/DataLayout.h"
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
    // TODO: what if Cont only has 1 predecessor too, does this happen when
    // the*/ handler block is trapping?*/
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

bool GEPOffsetsMatch(const GetElementPtrInst *GEP, Value *V,
                     const DataLayout &DL) {
  const GetElementPtrInst *Other = dyn_cast<GetElementPtrInst>(V);

  if (!GEP || !Other)
    return false;

  if (getUnderlyingObject(GEP) != getUnderlyingObject(Other))
    return false;

  return GEP->getPointerOffsetFrom(getUnderlyingObject(GEP), DL) ==
         Other->getPointerOffsetFrom(getUnderlyingObject(Other), DL);
}

bool matchesLHSorRHS(Value *LHS, Value *RHS, Value *V, const DataLayout &DL) {
  Value *LHSPtr, *RHSPtr;
  LoadInst *L = dyn_cast<LoadInst>(V);

  if (match(LHS, m_Load(m_Value(LHSPtr)))) {
    LoadInst *LHSLoad = dyn_cast<LoadInst>(LHSPtr);
    if (LHSLoad && L && L->getPointerOperand() == LHSLoad->getPointerOperand())
      return true;
  }

  if (match(RHS, m_Load(m_Value(RHSPtr)))) {
    LoadInst *RHSLoad = dyn_cast<LoadInst>(RHSPtr);
    if (RHSLoad && L && L->getPointerOperand() == RHSLoad->getPointerOperand())
      return true;
  }

  if (GetElementPtrInst *GEP = dyn_cast<GetElementPtrInst>(V))
    if (GEPOffsetsMatch(GEP, LHSPtr, DL) || GEPOffsetsMatch(GEP, RHSPtr, DL))
      return true;

  return V == LHSPtr || V == RHSPtr;
}

SmallVector<Instruction *> matchesBasePlusOffsetCompareToBase(Function &F) {
  SmallVector<Instruction *> MatchingInstructions;
  const DataLayout &DL = F.getParent()->getDataLayout();

  for (Instruction &I : instructions(F)) {
    Value *CmpOther;
    Value *LHS, *RHS;
    Value *OtherValue;
    ICmpInst::Predicate Pred;

    // Try to find patterns that match: if (a + b < a)
    // TODO: what if there is an overflow intrinsic but it isn't used in the
    // non-overflow/overflow scheme? It's just sort of "there" and not handled
    // by UBSAN or other.
    if (match(&I, m_c_ICmp(Pred,
                           m_ExtractValue<0>(m_CombineOr(
                               m_Intrinsic<Intrinsic::uadd_with_overflow>(
                                   m_Value(LHS), m_Value(RHS)),
                               m_Intrinsic<Intrinsic::sadd_with_overflow>(
                                   m_Value(LHS), m_Value(RHS)))),
                           m_Value(CmpOther)))) {

      // Predicates like >=, <=, ==, and != don't match the idiom
      if (ICmpInst::isNonStrictPredicate(Pred) || Pred == ICmpInst::ICMP_EQ ||
          Pred == ICmpInst::ICMP_NE)
        continue;

      // The commutative nature of m_c_ICmp means a predicate matching our
      // pattern should be {U,S}LT.
      if (Pred != ICmpInst::Predicate::ICMP_ULT &&
          Pred != ICmpInst::Predicate::ICMP_SLT)
        continue;

      if (CmpOther == LHS || CmpOther == RHS) {
        MatchingInstructions.push_back(&I);
        continue;
      }

      if (match(CmpOther, m_Load(m_Value(OtherValue)))) {
        if (matchesLHSorRHS(LHS, RHS, OtherValue, DL)) {
          MatchingInstructions.push_back(&I);
          continue;
        }
      }

      if (PHINode *Phi = dyn_cast<PHINode>(CmpOther)) {
        if (Phi->getNumIncomingValues() != 2)
          continue; // no match
        LoadInst *Phi0 = dyn_cast<LoadInst>(Phi->getIncomingValue(0));
        LoadInst *Phi1 = dyn_cast<LoadInst>(Phi->getIncomingValue(1));
        if (matchesLHSorRHS(LHS, RHS, Phi0->getPointerOperand(), DL) ||
            matchesLHSorRHS(LHS, RHS, Phi1->getPointerOperand(), DL)) {
          MatchingInstructions.push_back(&I);
          continue;
        }
      }
    }
  }

  return MatchingInstructions;
}

} // namespace

PreservedAnalyses IdiomExclusionsPass::run(Function &F,
                                           FunctionAnalysisManager &AM) {
  errs() << "Running IdiomExclusionsPass\n";
  SmallVector<Instruction *> MatchingInstructions =
      matchesBasePlusOffsetCompareToBase(F);
  for (Instruction *I : MatchingInstructions)
    removeEdgeToOverflowHandler(I);

  return PreservedAnalyses::none();
}
