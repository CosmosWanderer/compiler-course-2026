#include "mlir/Dialect/Affine/Analysis/LoopAnalysis.h"
#include "mlir/Dialect/Affine/IR/AffineOps.h"
#include "mlir/Dialect/LLVMIR/LLVMDialect.h"
#include "mlir/IR/PatternMatch.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Tools/Plugins/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"

using namespace mlir;

namespace {
class PotashnikPass
    : public PassWrapper<PotashnikPass, OperationPass<ModuleOp>> {
public:
  StringRef getArgument() const final { return "trip-count"; }
  StringRef getDescription() const final { return "Adds trip_count for loops"; }

  void runOnOperation() override {
    ModuleOp mod_op = getOperation();

    mod_op.walk([&](affine::AffineForOp for_op) {
      std::optional<uint64_t> trip_count_opt =
          affine::getConstantTripCount(for_op);
      if (!trip_count_opt)
        return;
      uint64_t trip_count = *trip_count_opt;
      if (trip_count >
          static_cast<uint64_t>(std::numeric_limits<int64_t>::max())) {
        return;
      }
      for_op->setAttr(
          "trip_count",
          IntegerAttr::get(IntegerType::get(for_op.getContext(), 64),
                           static_cast<int64_t>(trip_count)));
    });
  }
};
} // namespace

MLIR_DECLARE_EXPLICIT_TYPE_ID(PotashnikPass)
MLIR_DEFINE_EXPLICIT_TYPE_ID(PotashnikPass)

mlir::PassPluginLibraryInfo getPotashnikPassPluginInfo() {
  return {MLIR_PLUGIN_API_VERSION, "PotashnikPass", "1.0",
          []() { mlir::PassRegistration<PotashnikPass>(); }};
}

extern "C" LLVM_ATTRIBUTE_WEAK mlir::PassPluginLibraryInfo
mlirGetPassPluginInfo() {
  return getPotashnikPassPluginInfo();
}
