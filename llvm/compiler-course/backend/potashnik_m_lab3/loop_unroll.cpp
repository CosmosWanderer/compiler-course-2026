#include "X86.h"
#include "X86InstrInfo.h"
#include "X86Subtarget.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineLoopInfo.h"

#include <algorithm>

using namespace llvm;

namespace {

class PotashnikUnroller {
  static constexpr int max_factor = 5;
  static constexpr int max_trip = 1024;

public:
  const X86InstrInfo *TII = nullptr;

  void get_all_loops(MachineLoop *loop, SmallVectorImpl<MachineLoop *> &loops) {
    for (MachineLoop *sub_loop : *loop)
      get_all_loops(sub_loop, loops);
    loops.push_back(loop);
  }

  bool try_unroll_loop(MachineLoop *L, MachineFunction &MF,
                       MachineLoopInfo &MLI) {
    MachineBasicBlock *loop_latch = L->getLoopLatch();
    MachineBasicBlock *exiting_block = get_unique_exiting_block(L);

    if (!only_one_preheader(L))
      return false;
    if (!loop_latch || !exiting_block)
      return false;
    if (exiting_block != loop_latch)
      return false;

    if (L->begin() != L->end())
      return false;

    int trip_count = get_trip_count(loop_latch);
    if (trip_count <= 1 || trip_count > max_trip)
      return false;

    int unroll_factor = choose_unroll_factor(trip_count, max_factor);
    if (unroll_factor <= 1)
      return false;

    SmallVector<MachineInstr *, 16> loop_body = collect_loop_body(L, MLI);
    if (loop_body.empty())
      return false;

    MachineBasicBlock::iterator induction_instr =
        find_induction_add(loop_latch);
    if (induction_instr == loop_latch->end())
      return false;

    // Вставка копий тела перед индукционной инструкцией
    int copies_left = unroll_factor - 1;
    while (copies_left--) {
      for (MachineInstr *MI : loop_body) {
        MachineInstr *cloned = MF.CloneMachineInstr(MI);
        loop_latch->insert(induction_instr, cloned);
      }
    }
    return true;
  }

private:
  MachineBasicBlock::iterator find_induction_add(MachineBasicBlock *MBB) const {
    for (auto MI = MBB->begin(), ME = MBB->end(); MI != ME; ++MI)
      if (MI->getOpcode() == X86::ADD32ri8)
        return MI;
    return MBB->end();
  }

  MachineBasicBlock *get_unique_exiting_block(MachineLoop *L) const {
    MachineBasicBlock *exiting = nullptr;
    for (MachineBasicBlock *MBB : L->blocks()) {
      bool has_outside_succ = false;
      for (MachineBasicBlock *succ : MBB->successors()) {
        if (!L->contains(succ)) {
          has_outside_succ = true;
          break;
        }
      }
      if (!has_outside_succ)
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

  int choose_unroll_factor(int trip, int max_factor_arg) const {
    int start_f = (max_factor_arg < trip) ? max_factor_arg : trip;
    for (int f = start_f; f > 1; --f) {
      if (trip % f == 0)
        return f;
    }
    return 1;
  }

  SmallVector<MachineInstr *, 16>
  collect_loop_body(MachineLoop *L, MachineLoopInfo &MLI) const {
    SmallVector<MachineInstr *, 16> body;
    for (MachineBasicBlock *MBB : L->blocks()) {
      if (MLI.getLoopFor(MBB) != L)
        continue;
      for (MachineInstr &MI : *MBB) {
        if (MI.isBranch() || MI.isTerminator() || MI.isDebugInstr() ||
            MI.getOpcode() == X86::CMP32ri8 || MI.getOpcode() == X86::CMP32ri)
          continue;
        body.push_back(&MI);
      }
    }
    return body;
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