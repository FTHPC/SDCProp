#ifndef SDCPROP_H
#define SDCPROP_H

#include <string>
#include <iostream>
#include <vector>
#include <assert.h>
#include <stdlib.h>
#include <map>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
#include <unistd.h>
#include <fstream>
using std::ifstream;
#include <cxxabi.h>

 
 
 
 
 
//#include <llvm/IR/DebugInfo.h>
 
#include <llvm/Transforms/Utils/ValueMapper.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/Constants.h>
using namespace llvm;

#include "FlipIt/pass/faults.h"
using namespace FlipIt;

#include "Types.h"


namespace {
  class SDCProp : public ModulePass {
    public:
        static char ID;
        SDCProp() : ModulePass(ID) {
            
            // function pointers
            BBstart = BBend = logAlloca = dummyFree = checkCmp = NULL;
            storeInt = storeFloat = storeDouble = storePtr = NULL;
            loadInt = loadFloat = loadDouble = loadPtr = NULL;
            
            // basic data types
            i64Ty = Type::getInt64Ty(getGlobalContext());
            i32Ty = Type::getInt32Ty(getGlobalContext());
            floatTy = Type::getFloatTy(getGlobalContext());
            doubleTy = Type::getDoubleTy(getGlobalContext());
            ptrTy = Type::getInt8PtrTy(getGlobalContext());
            voidTy = Type::getVoidTy(getGlobalContext());
            gvar_fname = NULL;
        }

        virtual bool runOnModule(Module &M);

    private:
        int initInstrumentation();
        int getStartID(int size, const char* path);
        bool tryLock(const char* lockName);
        void releaseLock(int lock, const char* lockName);
        Function* createProtoType(Function* F, std::string suffix = "_F", int nRepeat = 1);
        Function* createFaultyFunction(Function* gold, std::string suffix = "_F", int nRepeat = 1);
        void readConfig(std::string filename);
        void copyAndUpdateGlobals();
        std::string demangle(std::string name);
        
        bool instrumentBB(BasicBlock*& origBB, BasicBlock* copyBB);
        bool instrumentStore(Instruction* I, Instruction* J);
        bool instrumentLoad(Instruction* I, Instruction* J, std::map<Value*, Value*>& replace);
        void handleMalloc(Instruction* I, Instruction* J, Instruction* Inext, std::string name);
        void handleRealloc(Instruction* I, Instruction* J, Instruction* Inext, std::string name);
        bool handleAlloc(Instruction* I, Instruction* J, Instruction* Inext);
        bool instrumentCall(Instruction* I, Instruction* J, Instruction* Inext, std::map<Value*, Value*>&replace, std::vector<Instruction*>& toErase);
        bool instrumentCompare(Instruction* I, Instruction* J, Instruction* Inext);
        
        bool copyMetadata(Instruction* New, Instruction* Old);
        void createMask(); 
        Instruction* convertType(Value* from, Type* to, Instruction* insertBefore);
        void removeAllocas(int num, Instruction* Inext);
        void handleSwitch(Instruction* newI, Instruction* oldI, std::vector<Instruction*>& toErase);
        bool handleInvoke(Instruction* dup, Instruction* orig, Instruction* Inext, std::vector<Instruction*>& toErase);
        void createFileNameGlobal(std::string fname);
        
    
        Module* M;
        std::vector<Function*> goldFunc;
        std::vector<Function*> newFunc;
        std::vector<std::string> dupFuncs;
        std::vector<std::string> singleFuncs;
        std::vector<Function*> faultyFuncs;
        int BB_ID;
        DataLayout* Layout;
        GlobalVariable* loadMask;
        GlobalVariable* floatMask;
        GlobalVariable* doubleMask;
        GlobalVariable* ptrMask;
        std::vector<Instruction*> flipitInsts;
        std::vector<Instruction*> phiInsts;
        Type* i64Ty;
        Type* i32Ty;
        Type* floatTy;
        Type* doubleTy;
        Type* ptrTy;
        Type* voidTy;
        std::string flipitConfig; 
        bool isMain;
        int nAllocas;
        std::map<std::string, std::vector<std::pair<GlobalVariable*, GlobalVariable*>>*> gblVar;
        Constant*  gvar_fname; 
        Constant*  fname_data; 

        // Instrumentation Calls
        Value* storeVecElm;
        Value* loadVecElm;
        Value* BBstart;
        Value* BBend;
        Value* storeInt;
        Value* storeFloat;
        Value* storeDouble;
        Value* storePtr;
        Value* loadInt;
        Value* loadFloat;
        Value* loadDouble;
        Value* loadPtr;
        Value* dummyFree;
        Value* logAlloca;
        Value* rmAlloca;
        Value* logMalloc; 
        Value* logRealloc; 
        Value* checkCmp;
        Value* relateAlloca;
        Value* logCmdArgs;
        Value* startCallLogging;
        Value* relateCallMallocs;
        Value* logGbl;
        Value* startFunc;
    };
}

char SDCProp::ID = 0;
static RegisterPass<SDCProp> SDCP("SDCProp", "Pass that tracks SDC Propagation", false, false);
#endif
