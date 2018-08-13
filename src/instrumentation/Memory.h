#ifndef MEMORY_H
#define MEMORY_H

#include "MemoryAccess.h"
#include "Types.h"

#include <map>
#include <vector>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stack>
#include <math.h>
#include <mpi.h>

#include "FlipIt/corrupt/corrupt.h"
struct MemAlloc
{
    void* gbase;
    void* fbase;
    size_t size;
    void* trace;
    int id;
};

struct MemValue
{
    unsigned long data;
    char type;
    bool dev;
};

extern char  etext, edata, end; 

class Memory
{
    public:
        Memory() { outsideNum = 0; insideNum = 0; afterNum = 0; beforeNum = 0; bothNum = 0; }
        bool load(unsigned long address, unsigned long* data, char type);
        bool store(unsigned long address, unsigned long data, char type);
        bool goldLoad(unsigned long address, unsigned long data, char type, bool dev);
        char existsInFaulty(unsigned long address, int elmSize);
        unsigned long getFaulty(unsigned long addr);
        void logAlloc(void* fptr, void* gptr, unsigned long size, void* trace, int type, char* file, int line, int id = -1);
        void logRealloc(void* fptr, void* gptr, void* old_fptr, void* old_gptr, unsigned long size, void* trace, int type, char* file, int line, int id = -1);
        void logRelateStackAlloc(void* fptr, void* gtr, unsigned long size, void* trace, char* file, int line, int id);
        int getStackAllocID(void* ptr);
        void getCorruption(const void* buf, const int count, const int size,
            void** data, int** idxs, int* num);
        void setCorruption(const void* buf, const int count, const char size,
            const void* data, const int* idxs, const int num);
        int freeAlloc(void* ptr);
        void copyMem2Faulty(int mode);
        void printStats(FILE*, long ld, long st);
        std::map<unsigned long, unsigned char>* getMem()
            {   return &memory;  }
        void removeAlloca(int num);
        void relateLoggedMallocs(char single);
        void logMallocInCall(void* ptr, unsigned long size, void* trace)
        {
            MemAlloc* ma = new MemAlloc;
            ma->gbase = ptr;
            ma->fbase = ptr;
            ma->size = size;
            ma->trace = trace;
            ma->id = -1;
            callAllocs.push_back(ma);
        }
        void logMemoryGlobal(unsigned char* fPtr, unsigned char* gPtr, unsigned long size);
        void createNewFuncAllocCounter() {
            nAllocsPerFunc.push(0);
        }
        //std::map<void*, MemAlloc*>* getAllocs() { return &allocs; }
        //std::map<void*, MemAlloc*>* getStackAllocs() { return &stackAllocs; }
    private:
        std::map<unsigned long, unsigned char> memory;
        std::map<void*, std::pair<char*,int> > addr2line;
        std::vector<MemAlloc*> allocs;
        std::vector<MemAlloc*> stackAllocs;
        std::vector<MemAlloc*> callAllocs;
        std::stack<int> nAllocsPerFunc;
        //ld, st overlap classifications
        static const char contained;
        static const char after;
        static const char before;
        static const char both;
        static const char outside;

        // counters for types of loads/stores
        unsigned long outsideNum;
        unsigned long insideNum;
        unsigned long afterNum;
        unsigned long beforeNum;
        unsigned long bothNum;
};
#endif
