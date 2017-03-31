
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include "llvm/IR/DebugInfo.h"
#include <string>
#include "llvm/Support/raw_ostream.h"

using namespace std;
using namespace llvm;

extern "C" {


#define PATHPROF(X) PathPrOfIlEr_ ## X

// This macro allows us to prefix strings so that they are less likely to
// conflict with existing symbol names in the examined programs.
// e.g. EPP(entry) yields PaThPrOfIlInG_entry
#define EPP(X)  PaThPrOfIlInG_ ## X

// Implement your instrumentation functions here. You will probably need at
// least one function to log completed paths and one function to save the
// results to a file. You may wish to have others.

uint64_t parentbbId;
uint64_t state;
llvm::DenseMap <uint64_t,std::vector<uint64_t >> paths;//this maps the p[ath ID and respective bbIds in the path
llvm::DenseMap <uint64_t,llvm::DenseMap<uint64_t,uint64_t> > pathCount; //this maps the <function ID ,<path ID,the number of times path executed>>
extern uint64_t PATHPROF(numEdges);
extern uint64_t PATHPROF(totalNumPaths);

extern struct {
    uint64_t parentId;
    uint64_t childId;
    uint64_t encodingVal;
} PATHPROF(edgeInfo)[];

extern struct {
    uint64_t functionId;
    char* functionName;
    uint64_t pathId;
    uint64_t count;
} PATHPROF(pathInfo)[];


//void PATHPROF(incrementState)(uint64_t bbId){
////    if(parentbbId == 0){
////       parentbbId = bbId;
////       return;
////    }
////    for(auto i=0; i< PATHPROF(numEdges);i++){
////        if(PATHPROF(edgeInfo)[i].parentId == parentbbId && PATHPROF(edgeInfo)[i].childId == bbId){
////            cout<<"foundENCODING "<< PATHPROF(edgeInfo)[i].encodingVal<<"\n";
////            state = state + PATHPROF(edgeInfo)[i].encodingVal;
////        }
////        else{
////            state = 0;//intitialize state for a new path
////        }
////
////    }
////    parentbbId = bbId;
//}

//void PATHPROF(incrementState)(uint64_t edgeEncoding,uint64_t functionId,uint64_t isSuccesssorReturns){
// //   cout<<edgeEncoding<<","<<functionId<<","<<isSuccesssorReturns<<"\n";
//    //cout<<"state"<<state<<"\n";
//    if(isSuccesssorReturns){
//        //We have reached an endpoint of the path
//        auto foundFunction = pathCount.find(functionId);
//        if(foundFunction == pathCount.end())
//        {   //Function is also new
//            //cout<<"funcFound"<<functionId;
//            llvm::DenseMap<uint64_t ,uint64_t > funcPathCount;
//            funcPathCount.insert(std::make_pair(state,1));
//            pathCount.insert(std::make_pair(functionId,funcPathCount));
//        }
//        else{
//            //Functioin is there.Lets check whether the path is already there
//            auto pathsMap = foundFunction->second;
//            auto foundPath = pathsMap.find(state);
//            if(foundPath == pathsMap.end()){
//                //Path is not yet traversed.This is the first time
//                pathsMap.insert(std::make_pair(state,1));
//                pathCount.find(functionId)->second = pathsMap;
//            }
//            else{
//                //Path is also there.just increment the count
//                //pathCount.find(functionId)->second.find(state)->second++;
//                pathCount.find(functionId)->second.find(state)->second = pathCount.find(functionId)->second.find(state)->second + 1;
//            }
//        }
//
//        //Updating the data on external structure table
//        for(int i= 0; i<PATHPROF(totalNumPaths);i++){
//            if(PATHPROF(pathInfo)[i].functionId == functionId && PATHPROF(pathInfo)[i].pathId == state){
//                PATHPROF(pathInfo)[i].count = pathCount.find(functionId)->second.find(state)->second;
//            }
//
//        }
//
//        state = 0;
//    }
//    else{
//        state=state+edgeEncoding;
//
//    }
//}

void PATHPROF(resetState)(uint64_t functionId){

    for(int i= 0; i<PATHPROF(totalNumPaths);i++){
        if(PATHPROF(pathInfo)[i].functionId == functionId && PATHPROF(pathInfo)[i].pathId == state){
            PATHPROF(pathInfo)[i].count = PATHPROF(pathInfo)[i].count+1;
        }
    }
      state = 0;
}

void PATHPROF(incrementState)(uint64_t edgeEncoding,uint64_t functionId,uint64_t isSuccesssorReturns){
    //   cout<<edgeEncoding<<","<<functionId<<","<<isSuccesssorReturns<<"\n";
    //cout<<"state"<<state<<"\n";
  //  if(isSuccesssorReturns){
    state=state+edgeEncoding;

    //Updating the data on external structure table
//        for(int i= 0; i<PATHPROF(totalNumPaths);i++){
//            if(PATHPROF(pathInfo)[i].functionId == functionId && PATHPROF(pathInfo)[i].pathId == state){
//                PATHPROF(pathInfo)[i].count = PATHPROF(pathInfo)[i].count+1;
//            }
//        }
      //  state = 0;
//    }
//    else{
//        state=state+edgeEncoding;
//
//    }
}


void PATHPROF(print)() {
    //  cout<<"numEdgges"<<PATHPROF(numEdges)<<"\n";
//    std::ofstream myfile;
//    myfile.open ("path-counts.txt");
//    for (auto i = 0; i < PATHPROF(numEdges); i++) {
//        cout << "pId" << PATHPROF(edgeInfo)[i].parentId << "childId" << PATHPROF(edgeInfo)[i].childId << ","
//             << PATHPROF(edgeInfo)[i].encodingVal << "\n";
//
//    }
//    myfile.close();

    std::ofstream myfile;
    myfile.open ("path-profile-results.txt");
    for (auto i = 0; i < PATHPROF(totalNumPaths); i++) {
        if(PATHPROF(pathInfo)[i].count !=0){
            cout << PATHPROF(pathInfo)[i].functionName << " " << PATHPROF(pathInfo)[i].pathId << " "
                 << PATHPROF(pathInfo)[i].count << "\n";
            myfile << PATHPROF(pathInfo)[i].functionName << " " << PATHPROF(pathInfo)[i].pathId << " "
                   << PATHPROF(pathInfo)[i].count << "\n";
        }

    }
    myfile.close();
}
//    std::ofstream myfile;
//    myfile.open ("path-counts.txt");

//    auto mapPathCount = pathCount;
//    //FILE *fp = fopen("path-profile-results.txt", "w");
//    for(auto &funcPath: mapPathCount){
//        for(auto path : funcPath.second){
//            cout<<"final"<<funcPath.first<<","<<path.first<<","<<path.second<<"\n";
//            auto func = funcPath.first;
//            auto pathId = path.first;
//            auto count = path.second;
//      //      myfile<<"profiled,"<<std::to_string(pathId)<<","<<std::to_string(count)<<"%";
//            //myfile<<"final"<<funcPath.first<<","<<path.first<<","<<path.second<<"\n";
//     //       fprintf(fp, "%s %lu %lu\n","profiled",pathId, count);
//        }
//
//    }
//    mapPathCount.clear();
//   // fclose(fp);
//}

}

