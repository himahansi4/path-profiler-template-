

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/Instruction.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/FileSystem.h"
#include <iostream>
#include <fstream>
#include <string>
#include "llvm/Support/raw_ostream.h"
#include <stdio.h>
#include <string.h>
#include <cstring>
#include <iostream>
#include "PathEncodingPass.h"

#include <unordered_map>

#include "ProfileDecodingPass.h"


using namespace llvm;
using namespace pathprofiling;
using namespace std;



namespace pathprofiling {
char ProfileDecodingPass::ID = 0;
}
llvm::DenseMap <uint64_t ,std::string > functions;
llvm::DenseMap <uint64_t ,u_int64_t > pathIds;
llvm::DenseMap <uint64_t ,u_int64_t > pathCounts;
llvm::DenseMap <uint64_t ,u_int64_t > maxPathCounts;
llvm::DenseMap<Edge, signed> edgeSet1;
std::vector <llvm::BasicBlock*> basicBlockSequence;


//llvm::DenseMap <std::string ,llvm::Function* > funcNames;

// Given a sequence of basic blocks composing a path, this function prints
// out the filename and line numbers associated with that path in CSV format.
void
printPath(std::ofstream &myfile, std::vector<llvm::BasicBlock*>& blocks) {
  unsigned line = 0;
  llvm::StringRef file;
  for (auto* bb : blocks) {
    for (auto& instruction : *bb) {
      llvm::DILocation* loc = instruction.getDebugLoc();
      if (loc && (loc->getLine() != line || loc->getFilename() != file)) {
        line = loc->getLine();
        file = loc->getFilename();
          myfile <<","<< file.str() << "," << std::to_string(line);
      }
    }
  }
}

void
getFileInfo(std::string infilename){
  string line;
  ifstream myfile (infilename);

  if (myfile.is_open())
  {
    auto i = 0;
    while ( getline (myfile,line) )
    {
      std::string::size_type sz;   // alias of size_t

      auto function = line.substr(0, line.find(' '));
     // int funcId_dec = std::stoi (functionId,&sz);
      functions.insert(std::make_pair(i,function));
      //errs()<<functionId<<"\n";

      auto index1 = line.find(' ');
      auto string2 = line.substr(index1);
      auto pathId = string2.substr(0, line.find(' '));
      int pathId_dec = std::stoi (pathId,&sz);
      pathIds.insert(std::make_pair(i,pathId_dec));
      //errs()<<pathId<<"\n";

      auto index2 = line.find_last_of(' ');
      auto count = line.substr(index2+1);
      int count_dec = std::stoi (count,&sz);
      pathCounts.insert(std::make_pair(i,count_dec));
      //errs()<<count<<"\n";
      i++;
    }
    myfile.close();
  }
  else cout << "Unable to open file";
}

void
ProfileDecodingPass::printPathDetails(uint64_t index, uint64_t count,Module &module){
  auto found = functions.find(index);
  for(auto &&f: module){
    if(found->second == f.getName()){
      //errs()<<"***"<<f.getName()<<"\n";
      basicBlockSequence.clear();
//      errs()<<"MaxPathCount "<<count<<"\n";
//      errs()<<"pathID "<<pathIds.find(index)->second<<"\n";
      decode(&f,pathIds.find(index)->second);
      std::ofstream outfile;
      outfile.open("top-five-paths.csv", std::ios_base::app);
      outfile<<count<<","<<found->second;
      auto sequence = basicBlockSequence;
//        errs()<<"sequence\n";
//      for(auto w : sequence){
//          errs()<<*w;
//      }
      printPath(outfile,sequence);
      outfile<<"\n";
      outfile.close();
    }
  }
}

bool
ProfileDecodingPass::runOnModule(Module &module) {
    getFileInfo(infilename);
//  for(auto g : pathCounts){
//    errs()<<g.first<<","<<g.second<<"\n";
//  }
    auto &encodePassCtx = getAnalysis<PathEncodingPass>();
    edgeSet1 =encodePassCtx.edges;
//    edgeSet1 = PathEncodingPass::getEdges();
//    for(auto ed : edgeSet1){
//        errs()<<*ed.first.first<<*ed.first.second<<ed.second<<"\n";
//    }
  std::ofstream myfile;
  myfile.open ("top-five-paths.csv");
  myfile.close();
  auto j=0;
  while(j<numberToReturn && pathCounts.size()>0){
    uint64_t  max =0;
    uint64_t index1 = 0;
   // errs()<<"max "<<max<<"\n";
    for(auto a :pathCounts){
  //    errs()<<"val "<<a.second<<"\n";
      if(max<=a.second){
        max=a.second;
        index1 = a.first;
      }
    }
 //   errs()<<"maxAfter"<<max<<"\n";
   // maxPathCounts.insert(std::make_pair(index1,max));
    printPathDetails(index1,max,module);
    pathCounts.erase(index1);
    j++;
  }

//    std::ofstream myfile;
//       myfile.open ("top-five-paths.csv");
//    myfile.close();
//   for(auto k : maxPathCounts){
//     errs()<<"maxp paths"<<k.first<<","<<k.second<<"\n";
//     auto found = functions.find(k.first);
//       for(auto &&f: module){
//          if(found->second == f.getName()){
//            errs()<<"***"<<f.getName()<<"\n";
//              basicBlockSequence.clear();
//              decode(&f,pathIds.find(k.first)->second);
//              std::ofstream outfile;
//              outfile.open("top-five-paths.csv", std::ios_base::app);
//              outfile<<k.second<<","<<found->second;
//              auto sequence = basicBlockSequence;
//           //   llvm::raw_ostream file1;
//              printPath(outfile,sequence);
//              outfile<<"\n";
//              outfile.close();
//          }
//        }
//   }
  return 0;
}
void
findPathRecursive(llvm::BasicBlock* bb,uint64_t currentSum){
    basicBlockSequence.push_back(bb);

  auto SI = succ_begin(bb);
  auto E = succ_end(bb);

  if(SI==E)
  {
    return ;
  }

  auto feasibleMaxEncoding = 0;
  BasicBlock* feasibleChild;
  for (;SI != E; ++SI) {
   //   errs()<<"child"<<**SI;
    auto edge =std::make_pair(bb,*SI);

//    for(auto ed : edgeSet1){
//    errs()<<*edge.first<<*edge.second;
//    }
   auto found = edgeSet1.find(edge);
    if(found != edgeSet1.end()){
     //   errs()<<"Edge"<<*found->first.first<<*found->first.second<<found->second<<"\n";
      auto encoding = found->second;
      if(feasibleMaxEncoding<=encoding && currentSum>=encoding){
      feasibleMaxEncoding = encoding;
      feasibleChild = *SI;
    }
    }

  }
   // errs()<<"End of for"<<"\n";
  currentSum = currentSum-feasibleMaxEncoding;
  //basicBlockSequence.push_back(feasibleChild);
 // errs()<<feasibleChild;
  findPathRecursive(feasibleChild,currentSum);
}

std::vector<llvm::BasicBlock*>
ProfileDecodingPass::decode(llvm::Function* function, uint64_t pathID) {
  std::vector<llvm::BasicBlock*> sequence;
  auto entryBlock = &function->getEntryBlock();
  //auto dag1 = PathEncodingPass::getDag();

 // for(int sum=0;sum<=pathID;){
  findPathRecursive(entryBlock,pathID);
 // }
  return sequence;
}

