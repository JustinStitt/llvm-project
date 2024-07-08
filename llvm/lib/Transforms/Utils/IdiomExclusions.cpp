#include "llvm/Transforms/Utils/IdiomExclusions.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/CodeGen/ISDOpcodes.h"
#include "llvm/IR/Analysis.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/PatternMatch.h"

using namespace llvm;
using namespace llvm::PatternMatch;

namespace {

enum class OverflowIdiomKind {
  BasePlusOffsetCompareToBase, /*    if(a + b u< a) {...} */
  WhileDec,                    /*    while(i--)    {...} */
};

struct OverflowIdiomInfo {
  OverflowIdiomKind Kind;
  WithOverflowInst *WOI;
  BasicBlock *Entry;
  BasicBlock *Overflow;
  BasicBlock *Cont;

  OverflowIdiomInfo() = delete;

  OverflowIdiomInfo(OverflowIdiomKind Kind, WithOverflowInst *WOI,
                    Instruction &I)
      : Kind(Kind), WOI(WOI) {
    Entry = WOI->getParent();
    BranchInst *Br = dyn_cast<BranchInst>(Entry->getTerminator());

    if (!Br || !Br->isConditional() || Br->getNumSuccessors() != 2)
      return;

    Cont = Br->getSuccessor(1);
    Overflow = Br->getSuccessor(0);

    BinaryOperator *BO = dyn_cast<BinaryOperator>(Br->getCondition());

    if (!BO || BO->getOpcode() != Instruction::Xor)
      return;

    if (auto *CI = dyn_cast<ConstantInt>(BO->getOperand(1))) {
      if (CI->isOne()) {
        Cont = Br->getSuccessor(0);
        Overflow = Br->getSuccessor(1);
      }
    }
  }
};

/// Remove the edge that connects the BasicBlock containing the overflow
/// intrinsic to the overflow-handling BasicBlock. Now there is a single edge
/// to the non-overflow handling BasicBlock.
void removeEdgeToOverflowHandler(const OverflowIdiomInfo &Info) {
  BasicBlock *Entry = Info.Entry;
  BasicBlock *Cont = Info.Cont;
  BasicBlock *Overflow = Info.Overflow;

  if (!Entry || !Cont || !Overflow)
    return;

  BranchInst::Create(Cont, Entry->getTerminator());
  // Handling overflows in non-recoverable modes means we have no edge to Cont
  if (Overflow->getUniqueSuccessor() == Cont)
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

  // It's possible that LHS or RHS underwent sign extension
  match(LHS, m_SExt(m_Value(LHS)));
  match(RHS, m_SExt(m_Value(RHS)));

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

SmallVector<OverflowIdiomInfo> matchesBasePlusOffsetCompareToBase(Function &F) {
  SmallVector<OverflowIdiomInfo> MatchedOverflowIdioms;
  const DataLayout &DL = F.getParent()->getDataLayout();

  for (Instruction &I : instructions(F)) {
    Value *CmpOther;
    Value *LHS, *RHS;
    Value *OtherValue;
    WithOverflowInst *WOI;
    ICmpInst::Predicate Pred;

    // Try to find patterns that match: if (a + b < a)
    // TODO: what if there is an overflow intrinsic but it isn't used in the
    // non-overflow/overflow scheme? It's just sort of "there" and not handled
    // by UBSAN or other. Essentially, how can we ensure that the Overflow
    // handling BasicBlock is actually handling the overflow?
    if (match(&I, m_c_ICmp(Pred, m_ExtractValue<0>(m_WithOverflowInst(WOI)),
                           m_Value(CmpOther)))) {

      if (!match(WOI, m_Intrinsic<Intrinsic::uadd_with_overflow>(m_Value(LHS),
                                                                 m_Value(RHS))))
        continue;

      // Predicates like >=, <=, ==, and != don't match the idiom
      if (ICmpInst::isNonStrictPredicate(Pred) || Pred == ICmpInst::ICMP_EQ ||
          Pred == ICmpInst::ICMP_NE)
        continue;

      // The commutative nature of m_c_ICmp means a predicate matching our
      // pattern should be ULT.
      if (Pred != ICmpInst::Predicate::ICMP_ULT)
        continue;

      OverflowIdiomInfo Info(OverflowIdiomKind::BasePlusOffsetCompareToBase,
                             WOI, I);

      if (CmpOther == LHS || CmpOther == RHS) {
        MatchedOverflowIdioms.push_back(std::move(Info));
        continue;
      }

      if (match(CmpOther, m_Load(m_Value(OtherValue)))) {
        if (matchesLHSorRHS(LHS, RHS, OtherValue, DL)) {
          MatchedOverflowIdioms.push_back(std::move(Info));
          continue;
        }
      }

      if (PHINode *Phi = dyn_cast<PHINode>(CmpOther)) {
        if (Phi->getNumIncomingValues() != 2)
          continue;
        Value *Val0 = Phi->getIncomingValue(0);
        Value *Val1 = Phi->getIncomingValue(1);
        match(Val0, m_SExt(m_Value(Val0)));
        match(Val1, m_SExt(m_Value(Val1)));
        LoadInst *Phi0 = dyn_cast<LoadInst>(Val0);
        LoadInst *Phi1 = dyn_cast<LoadInst>(Val1);
        if (!Phi0 || !Phi1)
          continue;
        if (matchesLHSorRHS(LHS, RHS, Phi0->getPointerOperand(), DL) ||
            matchesLHSorRHS(LHS, RHS, Phi1->getPointerOperand(), DL)) {
          MatchedOverflowIdioms.push_back(std::move(Info));
          continue;
        }
      }
    }
  }

  return MatchedOverflowIdioms;
}

} // namespace

PreservedAnalyses IdiomExclusionsPass::run(Function &F,
                                           FunctionAnalysisManager &AM) {
  SmallVector<OverflowIdiomInfo> OverflowIdioms =
      matchesBasePlusOffsetCompareToBase(F);

  if (OverflowIdioms.empty())
    return PreservedAnalyses::all();

  for (const OverflowIdiomInfo &Info : OverflowIdioms)
    removeEdgeToOverflowHandler(Info);

  return PreservedAnalyses::none();
}
