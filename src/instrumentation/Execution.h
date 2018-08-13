#ifndef EXECUTION_H
#define EXECUTION_H

#include <vector>
#include <map>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sstream>

#include "Types.h"
#include "BasicBlock.h"
#include "Memory.h"

#include "mpi.h"

class Execution
{
    public:
        Execution(); 
        ~Execution() { if (outfile !=NULL)
                            fclose(outfile);
                    }
        void printStats(long ld, long st);
        void dumpMemory(const char* fname);
        /*void printStats();
        void dumpMemory(int startBB, int endBB, const char* gMem, const char* fMem);
        void addToDumpTable(std::map<unsigned long, unsigned short> &table, MemoryAccess_t* MA);
        void dumpTable(std::map<unsigned long, unsigned short> &table,\
                        const char* fname);
        void computeAllocStats(std::map<unsigned long, unsigned short>& gTable,
                        std::map<unsigned long, unsigned short>& fTable);
        void printAllocStats(std::map<unsigned long, unsigned short>& gTable,
                    std::map<unsigned long, unsigned short>& fTable,
                    std::map<void*, MemAlloc*>* allocs);
        void addGoldBB(BasicBlock* BB) { exeGold = true; gold.push_back(BB); }
        void addFaultyBB(BasicBlock* BB);
        int getNumGoldBB() { return gold.size(); }
        int getNumFaultyBB() { return faulty.size(); }
        BasicBlock* getGoldBB(int i);
        BasicBlock* getFaultyBB(int i);
        MemoryAccess_t* getCorrespondingAccess();
        int getNumInjections();
        bool getDiverged();
        int getNumPerturbedLoads();
        int getNumPerturbedStores();
        double getPercentPerturbedLoads();
        double getPercentPerturbedStores();
        */
        Memory* getMem() { return &mem; }
        int logStackAlloc(unsigned char* fptr, unsigned char* gptr, unsigned int size, int type, char* fname, int line);

    private:

        FILE* outfile;
        bool exeGold;
        unsigned int nL, nS, npL, npS;
        unsigned int DISP_I;
        Memory mem;
        //std::vector<BasicBlock*> gold;
        //std::vector<BasicBlock*> faulty;
};
#endif
