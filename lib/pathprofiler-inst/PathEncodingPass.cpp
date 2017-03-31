

#include "llvm/ADT/PostOrderIterator.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/GraphWriter.h"
#include "InnermostLoops.h"
#include "llvm/IR/BasicBlock.h"
#include "PathProfilingPass.h"
#include "PathEncodingPass.h"
#include "PathProfilingPass.h"
#include "InnermostLoops.h"
#include "llvm/IR/CFG.h"
#include "llvm/ADT/GraphTraits.h"
#include <algorithm>
#include <vector>
#include <stack>
#include <queue>
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/CallSite.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"
#include "llvm/Support/raw_ostream.h"




using namespace llvm;
using namespace pathprofiling;


bool blockIsaFunction=false;
llvm::DenseSet<llvm::BasicBlock*> blockWithFunctions;
llvm::DenseMap<llvm::Function*, uint64_t > functionNumPaths;
uint64_t totalNumPaths = 0;
llvm::Function* mainFunction ;

namespace pathprofiling {
  char PathEncodingPass::ID = 0;
}

static llvm::Constant*
createConstantString(llvm::Module& m, llvm::StringRef str) {
    auto& context = m.getContext();

    auto* name    = llvm::ConstantDataArray::getString(context, str, true);
    auto* int8Ty  = llvm::Type::getInt8Ty(context);
    auto* arrayTy = llvm::ArrayType::get(int8Ty, str.size() + 1);
    auto* asStr   = new llvm::GlobalVariable(m, arrayTy, true, llvm::GlobalValue::PrivateLinkage, name);

    auto* zero = llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 0);
    llvm::Value* indices[] = {zero, zero};
    return llvm::ConstantExpr::getInBoundsGetElementPtr(arrayTy, asStr, indices);
}

void
PathEncodingPass::createPathsTable(Module &m) {
    auto &context = m.getContext();

    // Create the component types of the table
    auto *int64Ty    = Type::getInt64Ty(context);
    auto *stringTy   = Type::getInt8PtrTy(context);
    Type *fieldTys[] = {int64Ty,stringTy,int64Ty,int64Ty};
    auto *structTy   = StructType::get(context, fieldTys, false);
    auto *tableTy    = ArrayType::get(structTy, totalNumPaths);
    auto *numPathsGlobal = ConstantInt::get(int64Ty, totalNumPaths, false);
    new GlobalVariable(m, int64Ty, true, GlobalValue::ExternalLinkage,numPathsGlobal, "PathPrOfIlEr_totalNumPaths");

    std::vector<Constant*> values;
    for(auto numPaths: functionNumPaths){
      for(auto i=0; i<numPaths.second; i++){
          auto *functionId =  ConstantInt::get(int64Ty, functionIDs.find(numPaths.first)->second, false);
          const std::string  consFuncName = numPaths.first->getName();
          auto *funcName= createConstantString(m,StringRef(consFuncName));
          auto *pathId =  ConstantInt::get(int64Ty,i, false);
          auto *pathCount =  ConstantInt::get(int64Ty,0, false);
          Constant *structFields[] = {functionId,funcName,pathId,pathCount};
          values.push_back(ConstantStruct::get(structTy, structFields));
      }
    }
    auto *pathTable = ConstantArray::get(tableTy, values);
    new GlobalVariable(m, tableTy, false, GlobalValue::ExternalLinkage,pathTable, "PathPrOfIlEr_pathInfo");
}


void
PathEncodingPass::createEdgesTable(Module &m) {
    auto &context = m.getContext();

    // Create the component types of the table
    auto *int64Ty    = Type::getInt64Ty(context);
    Type *fieldTys[] = {int64Ty,int64Ty,int64Ty};
    auto *structTy   = StructType::get(context, fieldTys, false);
    auto *tableTy    = ArrayType::get(structTy, edges.size());
    auto *numEdgesGlobal = ConstantInt::get(int64Ty, edges.size(), false);
    new GlobalVariable(m, int64Ty, true, GlobalValue::ExternalLinkage,numEdgesGlobal, "PathPrOfIlEr_numEdges");

    // Compute and store an externally visible array of function information.
    std::vector<Constant*> values;
    for (auto &edge : edges) {
        auto *parentId= ConstantInt::get(int64Ty, basicBlockIDs.find(edge.first.first)->second, false);
        auto *childId = ConstantInt::get(int64Ty, basicBlockIDs.find(edge.first.second)->second, false);
        auto *edgeEncodingVal = ConstantInt::get(int64Ty, edge.second, false);
        Constant *structFields[] = {parentId,childId,edgeEncodingVal};
        values.push_back(ConstantStruct::get(structTy, structFields));
    }
    auto *edgeTable = ConstantArray::get(tableTy, values);
    new GlobalVariable(m, tableTy, false, GlobalValue::ExternalLinkage,edgeTable, "PathPrOfIlEr_edgeInfo");
}


void
PathEncodingPass::computeFunctionIDs(std::vector<Function*> &functions) {
    // errs()<<"**********FunctionIDList********\n";
    size_t nextID = 1;
    for (auto f : functions) {
        functionIDs[f] = nextID;
        errs()<<f->getName()<<","<<nextID<<"\n";
        nextID++;
    }
}

bool
PathEncodingPass::runOnModule(Module& module) {
   for (auto &f : module) {
       if (f.isDeclaration())	{
           continue;
       }
       handleFunction(&f);
       createAnnotations(&f);
     //  encode(f);
   }
    encode();

    std::vector<Function*> functionList;
    for (auto &f : module) {
        functionList.push_back(&f);

    }

    computeFunctionIDs(functionList);
    computeBasicBlockIDs(functionList);
    createEdgesTable(module);
    createPathsTable(module);
//    errs()<<"PathEncoding";
//    for(auto id : basicBlockIDs){
//        errs()<<"bb"<<*id.first<<","<<id.second<<"\n";
//
//    }

//
//    for(auto a : dag){
//        errs()<<"parent"<<*a.first;
//        for(auto b : a.second){
//            errs()<<("child")<<*b;
//        }
//    }
//    for(auto a : annotations){
//        errs()<<"exitAnnotations"<<*a.first<<","<<a.second<<"\n";
//    }
    for(auto a : edges){
        errs()<<"Encoding"<<*a.first.first<<","<<*a.first.second<<","<<a.second<<"\n";
    }

  return true;
}


void
PathEncodingPass::computeBasicBlockIDs(std::vector<Function*> &functions) {
    uint64_t nextID = 1;
    for (auto function: functions){
         llvm::DenseSet <uint64_t > funcBasicBlocks;
        for(auto &bb: *function ){
           // errs()<<bb<<","<<nextID<<"\n";
            basicBlockIDs.insert(std::make_pair(&bb,nextID));
            funcBasicBlocks.insert(nextID);
            nextID++;
        }
        functionBlockSet.insert(std::make_pair(function,funcBasicBlocks));
    }
}

void
PathEncodingPass::encode() {
//    for(auto f : functionIDs){
//        f.first->getEntryBlock();
//
//        auto SI = succ_begin(bb);
//        auto E = succ_end(bb);
//        for (;SI != E; ++SI) {
//
//        }
//    }
    for(auto kvPair : dag){
        auto SI = succ_begin(kvPair.first);
        auto E = succ_end(kvPair.first);
        std::queue <llvm::BasicBlock*> sortedChildren;
        for (;SI != E; ++SI) {
            sortedChildren.push((BasicBlock *&&) *SI);
        }
        uint64_t  encodeValue = 0;
        while(sortedChildren.size()>0){
            auto child2 = sortedChildren.front();
            edges.find(std::make_pair(kvPair.first,child2))->second = encodeValue;
           // errs()<<"annotations.find(child2)->second "<<annotations.find(child2)->second<<"\n";
            encodeValue = encodeValue+annotations.find(child2)->second;
            sortedChildren.pop();
        }
    }
}



//void
//PathEncodingPass::encode() {
//    for(auto kvPair : dag){
//        auto children = kvPair.second;
//        uint64_t  encodeValue = 0;
//        for(auto child : children){
//            edges.find(std::make_pair(kvPair.first,child))->second = encodeValue;
//            encodeValue = encodeValue+annotations.find(child)->second;
//        }
//    }
//}
//
//void
//PathEncodingPass::encode() {
//    for(auto kvPair : dag){
//        auto tempChildren = kvPair.second;
//      //  errs()<<"tempChildren "<<tempChildren.size()<<"\n";
//        std::queue <llvm::BasicBlock*> sortedChildren;
//        while (tempChildren.size()>0){
//            auto maxAnnotation = 0;
//            llvm::BasicBlock* maxAnnotatedChild ;
//            for(auto child1: tempChildren){
//                if(maxAnnotation<= annotations.find(child1)->second){
//                    maxAnnotation = annotations.find(child1)->second;
//                    maxAnnotatedChild = child1;
//                }
//            }
//            sortedChildren.push(maxAnnotatedChild);
//            tempChildren.erase(maxAnnotatedChild);
//        }
//       // errs()<<"sortedChildren "<<sortedChildren.size()<<"\n";
//        uint64_t  encodeValue = 0;
//       // errs()<<"parentBegin\n";
//        while(sortedChildren.size()>0){
//            auto child2 = sortedChildren.front();
//            edges.find(std::make_pair(kvPair.first,child2))->second = encodeValue;
//           // errs()<<"annotations.find(child2)->second "<<annotations.find(child2)->second<<"\n";
//            encodeValue = encodeValue+annotations.find(child2)->second;
//            sortedChildren.pop();
//        }
//    }
//}

void
PathEncodingPass::createAnnotations(llvm::Function *function){
    auto functionEntry = &function->getEntryBlock();
    auto found = dag.find(functionEntry);
    if(found != dag.end()){
        while(1==1){
            annotateBlocks(found->second);
            auto MainEntryAnnotated = annotations.find(functionEntry);
            if (MainEntryAnnotated != annotations.end()){
                functionNumPaths.insert(std::make_pair(function,MainEntryAnnotated->second));
                totalNumPaths = totalNumPaths+MainEntryAnnotated->second;
                break;
            }
        }
    }
}


void
PathEncodingPass::annotateBlocks(llvm::DenseSet<llvm::BasicBlock*> children){
    for(auto child: children){
        auto found = dag.find(child);
        if(found ==dag.end() ){
            annotations.insert(std::make_pair(child,1));
            for(auto edges : dag){
                uint64_t annotationSum = 0;
                for(auto child1 : edges.second){
                    auto found1 = annotations.find(child1);
                    if(found1 == annotations.end()){
                        annotationSum = 0;
                        break;
                    }
                    annotationSum = annotationSum+ found1->second;
                }
                if(annotationSum>0){
                    annotations.insert(std::make_pair(edges.first,annotationSum));
                }
            }
        }
        else{
            annotateBlocks(found->second);
        }
    }
}

void
PathEncodingPass::updateDag(llvm::BasicBlock *parent,llvm::BasicBlock *child){
        auto found = dag.find(parent);
        if(found == dag.end()){
            llvm::DenseSet <llvm::BasicBlock*> children;
            children.insert(child);
            dag.insert(std::make_pair(parent,children));
        }
        else{
            found->second.insert(child);
        }
}



void
PathEncodingPass::recordEdge(llvm::BasicBlock *bb, u_int64_t edgeCounter) {
    auto SI = succ_begin(bb);
    auto E = succ_end(bb);

    if(SI==E)
    {
        return;
    }
    for (;SI != E; ++SI) {
        recordEdge(*SI,edgeCounter);
        Edge edge = std::make_pair(bb,*SI);
        edges.insert(std::make_pair(edge,-1));
        updateDag(bb,*SI);
    }
}

void
PathEncodingPass::handleFunction(llvm::Function *f){
    llvm::DenseSet<llvm::BasicBlock*> latches;
    std::vector<llvm::Loop *> inloop;
    if (f->isDeclaration())	{
        return;
    }

    auto &LI = llvm::Pass::getAnalysis<LoopInfoWrapperPass>(*f).getLoopInfo();
    inloop = getInnermostLoops(LI);
    errs()<<"function "<<f->getName()<<"\n";
    for(auto a: inloop){
        return;
    }
    auto entryblock = &f->getEntryBlock();
    recordEdge(entryblock,0);
//    for(auto a : edges){
//        errs()<<"parent"<<*a.first.first<<"child"<<*a.first.second;
//    }

}
llvm::DenseMap <llvm::BasicBlock*, uint64_t >
PathEncodingPass::getBasicBlockIds(){
    return basicBlockIDs;
}

llvm::DenseMap <llvm::Function*, uint64_t >
PathEncodingPass::getFunctionIds(){
    return functionIDs;
}

//llvm::DenseMap<std::pair<llvm::BasicBlock*, llvm::BasicBlock*>, signed>
//PathEncodingPass::getEdges(){
//    return edges;
//}

//llvm::DenseMap<llvm::BasicBlock*,std::vector<llvm::BasicBlock*> >
//PathEncodingPass::getDag(){
//    return dag;




//};

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


std::vector<llvm::Loop*>
PathEncodingPass::getInnermostLoops(llvm::LoopInfo &li) {
    std::vector<llvm::Loop*> innermost;
    for (auto loop : li) {
        visitInnermostHelper(innermost, loop);
    }
    return innermost;
}




