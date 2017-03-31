//
// Created by himahansi on 06/03/17.
//

#ifndef PATHPROFILER_INNERMOSTLOOPS_H
#define PATHPROFILER_INNERMOSTLOOPS_H

#endif //PATHPROFILER_INNERMOSTLOOPS_H


#ifndef INNERMOSTLOOPS_H
#define INNERMOSTLOOPS_H

#include "llvm/Analysis/LoopInfo.h"
#include <vector>


namespace pathprofiling {

    std::vector<llvm::Loop*>
    getInnermostLoops(llvm::LoopInfo &li);

}


#endif
