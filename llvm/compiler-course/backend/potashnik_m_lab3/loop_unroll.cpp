#include "X86.h"
#include "X86InstrInfo.h"
#include "X86Subtarget.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineLoopInfo.h"

#include <algorithm>

using namespace llvm;

namespace {

static constexpr int max_unroll_factor = 5;
static constexpr int max_trip_count = 1024;

class PotashnikUnroller {
public:
  const X86InstrInfo *TII = nullptr;

  void get_all_loops(MachineLoop *loop, SmallVectorImpl<MachineLoop *> &loops) {
    for (MachineLoop *sub_loop : *loop)
      get_all_loops(sub_loop, loops);
    loops.push_back(loop);
  }

  bool try_unroll_loop(MachineLoop *L, MachineFunction &MF,
                       MachineLoopInfo &MLI) {
    MachineBasicBlock *latch = L->getLoopLatch();
    MachineBasicBlock *exiting = get_unique_exiting_block(L);

    if (!only_one_preheader(L) || !latch || !exiting || exiting != latch)
      return false;

    bool has_nested_loops = (L->begin() != L->end());
    if (has_nested_loops)
      return false;

    int trip_count = get_trip_count(latch);
    if (trip_count <= 1 || trip_count > max_trip_count)
      return false;

    int factor = choose_unroll_factor(trip_count, max_unroll_factor);
    if (factor <= 1)
      return false;

    SmallVector<MachineInstr *, 16> loop_body = collect_loop_body(L, MLI);
    if (loop_body.empty())
      return false;

    MachineBasicBlock::iterator induction_add = latch->end();
    for (auto MI = latch->begin(), ME = latch->end(); MI != ME; ++MI) {
      if (MI->getOpcode() == X86::ADD32ri8) {
        induction_add = MI;
        break;
      }
    }
    if (induction_add == latch->end())
      return false;

    for (int i = 0; i < factor - 1; ++i) {
      for (MachineInstr *MI : loop_body) {
        MachineInstr *cloned = MF.CloneMachineInstr(MI);
        latch->insert(induction_add, cloned);
      }
    }
    return true;
  }

private:
  MachineBasicBlock *get_unique_exiting_block(MachineLoop *L) const {
    MachineBasicBlock *exiting = nullptr;
    for (MachineBasicBlock *MBB : L->blocks()) {
      bool external_successor =
          llvm::any_of(MBB->successors(), [&](MachineBasicBlock *succ) {
            return !L->contains(succ);
          });

      if (!external_successor)
        continue;
      if (exiting)
        return nullptr;
      exiting = MBB;
    }
    return exiting;
  }

  bool only_one_preheader(MachineLoop *L) const {
    MachineBasicBlock *header = L->getHeader();
    if (!header)
      return false;
    MachineBasicBlock *preheader = nullptr;
    for (MachineBasicBlock *predecessor : header->predecessors()) {
      if (L->contains(predecessor))
        continue;
      if (preheader)
        return false;
      preheader = predecessor;
    }
    return preheader != nullptr;
  }

  int get_trip_count(MachineBasicBlock *latch) const {
    for (MachineInstr &MI : reverse(*latch)) {
      if (MI.getOpcode() != X86::CMP32ri8 && MI.getOpcode() != X86::CMP32ri)
        continue;
      for (MachineOperand &Op : MI.operands()) {
        if (Op.isImm())
          return static_cast<int>(Op.getImm());
      }
    }
    return -1;
  }

  int choose_unroll_factor(int trip_count, int max_factor) const {
    for (int F = std::min(max_factor, trip_count); F > 1; --F) {
      if (trip_count % F == 0)
        return F;
    }
    return 1;
  }

  SmallVector<MachineInstr *, 16>
  collect_loop_body(MachineLoop *L, MachineLoopInfo &MLI) const {
    SmallVector<MachineInstr *, 16> loop_body;
    for (MachineBasicBlock *MBB : L->blocks()) {
      if (MLI.getLoopFor(MBB) != L)
        continue;
      for (MachineInstr &MI : *MBB) {
        if (MI.isBranch() || MI.isTerminator() || MI.isDebugInstr())
          continue;
        unsigned opc = MI.getOpcode();
        if (opc == X86::CMP32ri8 || opc == X86::CMP32ri)
          continue;
        loop_body.push_back(&MI);
      }
    }
    return loop_body;
  }

  Register get_induction_reg(MachineBasicBlock *latch) const {
    for (MachineInstr &MI : *latch) {
      if (MI.getOpcode() != X86::ADD32ri8)
        continue;
      if (MI.getOperand(0).isReg())
        return MI.getOperand(0).getReg();
    }
    return Register();
  }
};

class PotashnikPass : public MachineFunctionPass {
public:
  static char ID;
  PotashnikPass() : MachineFunctionPass(ID) {}

  bool runOnMachineFunction(MachineFunction &MF) override {
    auto &MLI = getAnalysis<MachineLoopInfoWrapperPass>().getLI();

    PotashnikUnroller unroller;
    unroller.TII = MF.getSubtarget<X86Subtarget>().getInstrInfo();

    SmallVector<MachineLoop *, 8> loops;
    for (MachineLoop *loop : MLI)
      unroller.get_all_loops(loop, loops);

    bool changed = false;
    for (MachineLoop *loop : loops)
      changed |= unroller.try_unroll_loop(loop, MF, MLI);

    return changed;
  }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    MachineFunctionPass::getAnalysisUsage(AU);
    AU.addRequired<MachineLoopInfoWrapperPass>();
    AU.setPreservesCFG();
  }
};

} // namespace

char PotashnikPass::ID = 0;
static RegisterPass<PotashnikPass> X("PotashnikLoopUnroll", "LoopUnroll", false,
                                     false);