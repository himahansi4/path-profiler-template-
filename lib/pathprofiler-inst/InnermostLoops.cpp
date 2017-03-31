
#include "InnermostLoops.h"


using namespace llvm;
using namespace pathprofiling;



static void
visitInnermostHelper(std::vector<llvm::Loop*> &innermost,
                     llvm::Loop *loop) {
    if (loop->empty()) {
        innermost.push_back(loop);
    } else {
        for (auto &subloop : *loop) {
            visitInnermostHelper(innermost, subloop);
        }
    }
}


namespace pathprofiling {

    std::vector<llvm::Loop*>
    getInnermostLoops(llvm::LoopInfo &li) {
        std::vector<llvm::Loop*> innermost;
        for (auto loop : li) {
            visitInnermostHelper(innermost, loop);
        }
        return innermost;
    }

}
