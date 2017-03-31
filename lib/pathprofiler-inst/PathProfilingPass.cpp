

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Support/GraphWriter.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/IR/CallSite.h"
#include "PathProfilingPass.h"
#include "InnermostLoops.h"


using namespace llvm;
using namespace pathprofiling;



namespace pathprofiling {
char PathProfilingPass::ID = 0;
}

llvm::DenseMap <llvm::BasicBlock*,uint64_t > bbIdSet;
using Edge = std::pair<llvm::BasicBlock*, llvm::BasicBlock*>;
llvm::DenseMap<Edge, signed> edgeSet;
llvm::DenseMap<llvm::Function*,uint64_t > funcIds;



bool
PathProfilingPass::runOnModule(llvm::Module &module) {
//    std::vector<llvm::Loop *> inloop;
//    auto &LI = llvm::Pass::getAnalysis<LoopInfoWrapperPass>(*f).getLoopInfo();
//    inloop = getInnermostLoops(LI);

    auto &context = module.getContext();
    auto *int64Ty = Type::getInt64Ty(context);
    auto *stringTy   = Type::getInt8PtrTy(context);
    auto *voidTy  = Type::getVoidTy(context);

    auto *printer = module.getOrInsertFunction("PathPrOfIlEr_print", voidTy, nullptr);
    appendToGlobalDtors(module, llvm::cast<Function>(printer), 0);
//
//   //  Declare the counter function
    auto *helperTy1 = FunctionType::get(voidTy, {int64Ty,int64Ty,int64Ty}, false);
    auto *counterForInstruction  = module.getOrInsertFunction("PathPrOfIlEr_incrementState", helperTy1);

//    for(auto a : PathEncodingPass::getBasicBlockIds()){
//        errs()<<*a.first<<a.second;
//    }
//    std::vector<Function*> functionList;
//    for (auto &f : module) {
//        functionList.push_back(&f);
//
//    }
   funcIds= PathEncodingPass::getFunctionIds();

    bbIdSet=PathEncodingPass::getBasicBlockIds();
   // edgeSet = PathEncodingPass::getEdges();
    auto &encodePassCtx = getAnalysis<PathEncodingPass>();
    edgeSet =encodePassCtx.edges;
    for (auto &f : module) {
        if (f.isDeclaration())	{
            continue;
        }
        for(auto &bb: f){
            for(auto &i:bb){
                if(auto returnInst = dyn_cast<llvm::ReturnInst>(&i)){
                    auto founctionId = funcIds.find(bb.getParent())->second;
                    auto *int64Ty = Type::getInt64Ty(context);
                    auto *helperTy2 = FunctionType::get(voidTy, {int64Ty}, false);
                    auto *savePathandresetState = module.getOrInsertFunction("PathPrOfIlEr_resetState", helperTy2);
                    IRBuilder<> builder(&*bb.getFirstInsertionPt());
                    builder.CreateCall(savePathandresetState,{builder.getInt64(founctionId)});
                }
            }

        }
//        instrument(module,f,0,counterForInstruction);
    }
    instrumentWithSplitEdge(counterForInstruction);
//    for(auto a : edgeSet){
//       // errs()<<*a.first.first<<*a.first.second<<a.second;
//    }


    return true;


}





//void
//PathProfilingPass::instrumentWithSplitEdge(Value *stateIncrementer) {
//    for(auto a : edgeSet){
//        auto isSuccessoreturns = 0;
//         //errs()<<*a.first.first<<*a.first.second<<a.second;
//        auto intermediateBlock = llvm::SplitEdge(a.first.first,a.first.second);
//        auto founctionId = funcIds.find(a.first.first->getParent())->second;
//
//        for(auto &i: *a.first.second){
//            if(auto returnInst = dyn_cast<llvm::ReturnInst>(&i)){
//               // errs()<<"Return bb"<<*i.getParent();
//                isSuccessoreturns = 1;
//            }
//        }
//
//      //  errs()<<"intermediateBlock"<<*intermediateBlock;
//        IRBuilder<> builder(&*intermediateBlock->getFirstInsertionPt());
//        builder.CreateCall(stateIncrementer, {builder.getInt64(a.second),builder.getInt64(founctionId),builder.getInt64(isSuccessoreturns)});
//    }
//
//}


void
PathProfilingPass::instrumentWithSplitEdge(Value *stateIncrementer) {
    for(auto a : edgeSet){
        auto isSuccessoreturns = 0;
        //errs()<<*a.first.first<<*a.first.second<<a.second;
        auto intermediateBlock = llvm::SplitEdge(a.first.first,a.first.second);
        auto founctionId = funcIds.find(a.first.first->getParent())->second;
//
//        for(auto &i: *a.first.second){
//            if(auto returnInst = dyn_cast<llvm::ReturnInst>(&i)){
//                // errs()<<"Return bb"<<*i.getParent();
//                //isSuccessoreturns = 1;
//            }
//        }

        //  errs()<<"intermediateBlock"<<*intermediateBlock;
        IRBuilder<> builder(&*intermediateBlock->getFirstInsertionPt());
        builder.CreateCall(stateIncrementer, {builder.getInt64(a.second),builder.getInt64(founctionId),builder.getInt64(isSuccessoreturns)});
    }

}


//void
//PathProfilingPass::instrument(llvm::Module& module,
//                              llvm::Function& function,
//                              uint64_t functionID,
//                              Value *stateIncrementer) {
//    std::vector<llvm::Loop *> inloop;
//    auto &LI = llvm::Pass::getAnalysis<LoopInfoWrapperPass>(function).getLoopInfo();
//    inloop = pathprofiling::getInnermostLoops(LI);
//    for(auto a: inloop){
//        return;
//    }
//    for(auto &bb:function){
//        BasicBlock::iterator n;
//        n = bb.getFirstInsertionPt();
//
//        auto bbId =bbIds.find(&bb)->second;
//        IRBuilder<> builder(&bb);
//        builder.CreateCall(stateIncrementer, {builder.getInt64(bbId)});
////        for (auto &i:bb){
////
////
////            break;
////        }
//    }



//}

