
#ifndef PATHENCODINGPASS_H
#define PATHENCODINGPASS_H


#include "llvm/Analysis/LoopInfo.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"


namespace pathprofiling {
    using Edge = std::pair<llvm::BasicBlock*, llvm::BasicBlock*>;
    static  llvm::DenseMap<llvm::BasicBlock*, uint64_t >basicBlockIDs;
  //  static  llvm::DenseMap<llvm::Function*, uint64_t> functionIDs;
    static  llvm::DenseMap<llvm::Function*,llvm::DenseSet<uint64_t >> functionBlockSet;

    static llvm::DenseMap<llvm::BasicBlock*,llvm::DenseSet<llvm::BasicBlock*> >  dag;
    static llvm::DenseMap<llvm::Function*, uint64_t> functionIDs;

struct PathEncodingPass : public llvm::ModulePass {


  static char ID;

  static llvm::DenseMap<llvm::BasicBlock*, uint64_t> numPaths;
    llvm::DenseMap<Edge, signed> edges;


  llvm::DenseMap<llvm::BasicBlock*,uint64_t >  annotations;

  PathEncodingPass()
    : llvm::ModulePass(ID)
      { }

  void
  getAnalysisUsage(llvm::AnalysisUsage& au) const override {
    au.addRequired<llvm::LoopInfoWrapperPass>();
    au.setPreservesAll();
  }

  bool runOnModule(llvm::Module& m) override;

    std::vector<llvm::Loop*>   getInnermostLoops(llvm::LoopInfo &li);

  void encode();
  void handleFunction(llvm::Function *f);
    void handleInternalFunction(llvm::Function *f);
  void recordLoopEdge(llvm::BasicBlock *child,u_int64_t edgeCounter,llvm::DenseSet<llvm::BasicBlock*>);
    void recordEdge(llvm::BasicBlock *child,u_int64_t edgeCounter);
    void updateDag(llvm::BasicBlock *parent,llvm::BasicBlock *child);
    void createAnnotations(llvm::Function *function);
    void annotateBlocks(llvm::DenseSet<llvm::BasicBlock*> children);
   // void computeFunctionIDs(std::vector<llvm::Function*> &functions);
    void computeBasicBlockIDs(std::vector<llvm::Function*> &functions);
    void createEdgesTable(llvm::Module &m);
    void createPathsTable(llvm::Module &m);
    void computeFunctionIDs(std::vector<llvm::Function*> &functions) ;
    static llvm::DenseMap <llvm::BasicBlock*, uint64_t > getBasicBlockIds();
    static llvm::DenseMap <llvm::Function*, uint64_t > getFunctionIds();
    static llvm::DenseMap<std::pair<llvm::BasicBlock*, llvm::BasicBlock*>, signed> getEdges();
    static llvm::DenseMap<llvm::BasicBlock*,std::vector<llvm::BasicBlock*> > getDag();

};


}  // namespace pathprofiling


#endif

