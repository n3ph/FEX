#include "Interface/IR/PassManager.h"
#include "Interface/Core/OpcodeDispatcher.h"

#include "LogManager.h"

namespace FEXCore::IR {

class SyscallOptimization final : public FEXCore::IR::Pass {
public:
  bool Run(OpDispatchBuilder *Disp) override;
};

bool SyscallOptimization::Run(OpDispatchBuilder *Disp) {
  bool Changed = false;
  auto CurrentIR = Disp->ViewIR();
  uintptr_t ListBegin = CurrentIR.GetListData();
  uintptr_t DataBegin = CurrentIR.GetData();

  auto Begin = CurrentIR.begin();
  auto Op = Begin();

  OrderedNode *RealNode = Op->GetNode(ListBegin);
  auto HeaderOp = RealNode->Op(DataBegin)->CW<FEXCore::IR::IROp_IRHeader>();
  LogMan::Throw::A(HeaderOp->Header.Op == OP_IRHEADER, "First op wasn't IRHeader");

  OrderedNode *BlockNode = HeaderOp->Blocks.GetNode(ListBegin);

  while (1) {
    auto BlockIROp = BlockNode->Op(DataBegin)->CW<FEXCore::IR::IROp_CodeBlock>();
    LogMan::Throw::A(BlockIROp->Header.Op == OP_CODEBLOCK, "IR type failed to be a code block");

    // We grab these nodes this way so we can iterate easily
    auto CodeBegin = CurrentIR.at(BlockIROp->Begin);
    auto CodeLast = CurrentIR.at(BlockIROp->Last);
    while (1) {
      auto CodeOp = CodeBegin();
      OrderedNode *CodeNode = CodeOp->GetNode(ListBegin);
      auto IROp = CodeNode->Op(DataBegin);

      if (IROp->Op == FEXCore::IR::OP_SYSCALL) {
        // Is the first argument a constant?
        uint64_t Constant;
        if (Disp->IsValueConstant(IROp->Args[0], &Constant)) {
          // XXX: We can now optimize this syscall
          //Changed = true;
        }

      }

      // CodeLast is inclusive. So we still need to dump the CodeLast op as well
      if (CodeBegin == CodeLast) {
        break;
      }
      ++CodeBegin;
    }

    if (BlockIROp->Next.ID() == 0) {
      break;
    } else {
      BlockNode = BlockIROp->Next.GetNode(ListBegin);
    }

  }

  return Changed;
}

FEXCore::IR::Pass* CreateSyscallOptimization() {
  return new SyscallOptimization{};
}

}
