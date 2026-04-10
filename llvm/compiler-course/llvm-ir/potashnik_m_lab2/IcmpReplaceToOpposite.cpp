#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"

namespace {
class PotashnikReplacer {
public:
  static void replace(llvm::ICmpInst *icmp) {
    llvm::CmpInst::Predicate new_pred = icmp->getInversePredicate();
    llvm::Value *left_val = icmp->getOperand(0);
    llvm::Value *right_val = icmp->getOperand(1);
    llvm::IRBuilder<> builder(icmp);
    llvm::Value *inversed = builder.CreateICmp(new_pred, left_val, right_val,
                                               icmp->getName() + ".inv");
    llvm::Value *negative_inversed =
        builder.CreateNot(inversed, inversed->getName() + ".not");
    icmp->replaceAllUsesWith(negative_inversed);
    icmp->eraseFromParent();
  }
};

struct PotashnikPass : llvm::PassInfoMixin<PotashnikPass> {
  llvm::PreservedAnalyses run(llvm::Function &func,
                              llvm::FunctionAnalysisManager &) {
    for (auto &bb : func) {
      for (auto &instruct : llvm::make_early_inc_range(bb)) {
        if (llvm::ICmpInst *icmp = llvm::dyn_cast<llvm::ICmpInst>(&instruct)) {
          PotashnikReplacer::replace(icmp);
        }
      }
    }
    return llvm::PreservedAnalyses::none();
  }

  static bool isRequired() { return true; }
};
} // namespace

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "PotashnikIcmpPass", "0.1",
          [](llvm::PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](llvm::StringRef name, llvm::FunctionPassManager &FPM,
                   llvm::ArrayRef<llvm::PassBuilder::PipelineElement>) -> bool {
                  if (name == "replace-icmp-to-opposite") {
                    FPM.addPass(PotashnikPass{});
                    return true;
                  }
                  return false;
                });
          }};
}
