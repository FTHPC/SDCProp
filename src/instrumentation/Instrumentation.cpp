#include "Instrumentation.h"
#include "MemoryAccess.h"
#include "BasicBlock.h"
#include "Execution.h"
#include "Memory.h"

#include <stdio.h>
#include <stdlib.h>
    
    extern char etext, edata, end;

Execution exe;
//Memory exe.getMem()->

int insideCall = 0;
int logCallMallocs = 0;
unsigned long DEV_LD = 0;
unsigned long DEV_STR = 0;
void* argBuf = NULL;

unsigned long long (*instCount)();
void setInstCount(unsigned long long (foo)()) {
    instCount = foo;
}

void copyMem2Faulty(int mode)
{
    exe.getMem()->copyMem2Faulty(mode);
}

int logMemoryAlloc(void* fptr, void* gptr, long size, void* trace, int type, char* file, int line)
{   
    insideCall = 1;
    //printf("Log Alloc: %d bytes at %p with trace %p\n", (int)size, fptr, trace);
    exe.getMem()->logAlloc(fptr, gptr, size, trace, type, file, line);
    insideCall = 0;
    return 1;
}

int logMemoryRealloc(void* fptr, void* gptr, void* old_fptr, void* old_gptr, long size, void* trace, int type, char* file, int line)
{   
    insideCall = 1;
    //printf("Log Alloc: %d bytes at %p with trace %p\n", (int)size, fptr, trace);
    exe.getMem()->logRealloc(fptr, gptr, old_fptr, old_gptr, size, trace, type, file, line);
    insideCall = 0;
    return 1;
}


int freeMemoryAlloc(void* ptr)
{   
    insideCall = 1;
    int ret = exe.getMem()->freeAlloc(ptr);
    insideCall = 0;
    return ret;
}

void dumpMemory(const char* fname)
{
    insideCall = 1;
    //exe.dumpMemory(fname);    
    insideCall = 0;
}

void printStats()
{
    insideCall = 1;
    int rank = -1;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank == 3)
        printf("Rank %d: Print StatsTotal Insts executed = %llu\n", rank, FLIPIT_GetExecutedInstructionCount());
    exe.printStats(DEV_LD, DEV_STR);
    insideCall = 0;
}

int inInstrumentationCall()
{
    return insideCall;
}
void logMallocInCall(void* ptr, unsigned long size, void* trace)
{
    insideCall = 1;
    printf("Log Call Alloc: %d bytes at %p with trace %p\n", (int)size,ptr, trace);
    exe.getMem()->logMallocInCall(ptr, size, trace);
    insideCall = 0;
}
void getCorruption(const void* buf, const int count, const int size, void** data,
    int** idxs, int* num)
{
    exe.getMem()->getCorruption(buf, count, size, data, idxs, num);  
}

void setCorruption(const void* buf, const int count, const char type, const void* data,
    const int* idxs, const int num)
{
    exe.getMem()->setCorruption(buf, count, type, data, idxs, num);  
}

/*****************************************************************************/
void __LOG_AND_RELATE_GLOBAL(unsigned char* fptr, unsigned char* gptr, unsigned long size)
{
    insideCall = 1;
    exe.getMem()->logMemoryGlobal(fptr, gptr, size);
    insideCall = 0;
}

void __LOG_MALLOCS_IN_CALL()
{
    logCallMallocs = 1;
}
void __RELATE_MALLOCS_IN_CALL(char type)
{
    logCallMallocs = 0;
    exe.getMem()->relateLoggedMallocs(type);
}

void __LOG_CMD_ARGS(int argc, char** argv)
{
    // array of char*
    exe.logStackAlloc((unsigned char*)argv, (unsigned char*)argv, argc*sizeof(char*), INT32, (char*) "main.c", 0);
    for(int i=0; i<argc; i++)
    {
        exe.logStackAlloc((unsigned char*)argv[i], (unsigned char*)argv[i], strlen(argv[i]), INT8, (char*) "main.c", 0);
    }

 //TODO: make seperate function and add call in SDCProp.cpp
    const char* fname = "../ERR_TOL.txt";
    if( access( fname, F_OK) != -1 ) {
        FILE* infile = fopen(fname,"r");
        fscanf(infile, "%lf", &DATA_TOL);
        fclose(infile);   
    }
    printf("SDCPROP: error tolerance = %g\n", DATA_TOL);

}
void __REMOVE_ALLOCA(int num)
{
    exe.getMem()->removeAlloca(num);
}

int __LOG_ALLOCA(unsigned char* ptr, unsigned long num, unsigned long size)
{
    insideCall = 1;
    printf("__LOG_ALLOCA\n");
    exit(1);
    //printf("Log Stack Alloca: %lu bytes at %p with trace %p\n", (int)size*num, ptr, ptr);
    //exe.logStackAlloc(ptr, ptr, size*num, NULL);
    insideCall = 0;
    return 1;

}

void __DUMMY_FREE(unsigned char* ptr)
{
    // Do nothing...
} 

unsigned long __GET_FAULTY_VALUE(unsigned long gaddr, unsigned long faddr, unsigned long gvalue)
{
    return exe.getMem()->getFaulty(gvalue);
}

int __LOG_AND_RELATE_ALLOCA(unsigned char* fPtr, unsigned char* gPtr,
                            unsigned long num, unsigned long size, int type,
                            char* fname, int line)
{
    insideCall = 1;
    int rank=0;// MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank == 0) {
        //printf("STACK_ALLOC (%p): %s:%d\n", fPtr, fname, line);
    }
    //printf("%p, %p, %u, %lu\n", fPtr, gPtr, num, size);
    //TODO: verify printf is needed and fix with correct trace arg
    //printf("Log Stack Alloca: %lu bytes at %p with trace %p\n", (int)size*num, gPtr, gPtr);
    exe.logStackAlloc(fPtr, gPtr, size*num, type, fname, line);
    insideCall = 0;
    return 1;
}

void __LOG_AND_RELATE_MALLOC(unsigned char* fPtr, unsigned char* gPtr,
            unsigned long size, int type, char* fname, int line)
{
    insideCall = 1;
    void *trace[16];
    int trace_size = 0;
    trace_size = backtrace(trace, 16);
    int rank; MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank == 0) {
        //printf("MEM_ALLOC: %s:%d\n", fname, line);
    }
    /* trace[1] is the location of the calling malloc function */
    /* messages[i] can let you know what executable to look in for .so */
    logMemoryAlloc(fPtr, gPtr, size, trace[1], type, fname, line);
    insideCall = 0;
}

void __LOG_AND_RELATE_REALLOC(unsigned char* fPtr, unsigned char* gPtr, unsigned char* old_fptr, unsigned char* old_gptr, 
            unsigned long size, int type, char* fname, int line)
{
    insideCall = 1;
    void *trace[16];
    int trace_size = 0;
    trace_size = backtrace(trace, 16);
    int rank; MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank == 0) {
        //printf("MEM_ALLOC: %s:%d\n", fname, line);
    }
    /* trace[1] is the location of the calling malloc function */
    /* messages[i] can let you know what executable to look in for .so */
    logMemoryRealloc(fPtr, gPtr, old_fptr, old_gptr, size, trace[1], type, fname, line);
    insideCall = 0;
}
void __CHECK_CMP(unsigned faulty, unsigned gold, unsigned BB_ID)
{
    insideCall = 1;
    if (faulty != gold) {
        int rank;
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        printf("Control Flow Divergence in static BB %u on rank %d\n", BB_ID, rank);
        printf("\n\nRank = %d\nDEV_STR = %lu\nDEV_LD = %lu\n", rank, DEV_STR, DEV_LD);
        printf("\n\nfaulty = %d\ngold = %d\n", faulty, gold);
        exe.printStats(DEV_LD, DEV_STR);
        raise(SIGUSR1);
        exit(1);
    }
    insideCall = 0;
}
void __START_FUNCTION()
{
    exe.getMem()->createNewFuncAllocCounter();
}



//=======================   Store   ===========================
void __STORE_INT(unsigned long fptr, unsigned long fvalue,
                unsigned long gptr, unsigned long gvalue,
                int nBytes, unsigned long mask)
{
    insideCall = 1;
    bool dev;
    if (nBytes == 1) {
        if (mask != 0) { shrinkMask(&mask, INT8); }
        fvalue ^= mask;
        dev = computeDeviation(fptr, fvalue, gptr, gvalue, INT8);
        exe.getMem()->store(fptr, fvalue, INT8);
    }
    else if (nBytes == 2) {
        if (mask != 0) { shrinkMask(&mask, INT16); }
        fvalue ^= mask;
        dev = computeDeviation(fptr, fvalue, gptr, gvalue, INT16);
        exe.getMem()->store(fptr, fvalue, INT16);
    }
    else if (nBytes == 4) {
        if (mask != 0) { shrinkMask(&mask, INT32); }
        fvalue ^= mask;
        dev = computeDeviation(fptr, fvalue, gptr, gvalue, INT32);
        exe.getMem()->store(fptr, fvalue, INT32);
    }
    else if (nBytes == 8) {
        fvalue ^= mask;
        dev = computeDeviation(fptr, fvalue, gptr, gvalue, INT64);
        exe.getMem()->store(fptr, fvalue, INT64);
    }
    else {
        printf("ERROR: Encounted an integer store of more than 8 bytes!!!\n");
    }
    if (dev) { DEV_STR++; }
    insideCall = 0;
}

void __STORE_FLOAT(unsigned long fptr, float fvalue, unsigned long gptr, float gvalue, float mask)
{
    insideCall = 1;
    unsigned long mData = 0;
    unsigned int* tmp = (unsigned int* ) &fvalue;
    //printf("FLT MASK = %x\n", *((unsigned*) &mask));
    mData = *tmp ^ *((unsigned int*) &mask);
    fvalue = *((float*)tmp);
    bool dev = computeDeviation(fptr, fvalue, gptr, gvalue, FLOAT);
    if (dev) { DEV_STR++; }

    // store value
    bool notInMemory = exe.getMem()->store(fptr, mData, FLOAT);
    insideCall = 0;
}

void __STORE_DOUBLE(unsigned long fptr, double fvalue, unsigned long gptr, double gvalue, double mask)
{
    insideCall = 1;
    unsigned long mData = 0;
    unsigned long* tmp = (unsigned long* ) &fvalue;
    //printf("DBL MASK = %g\n", mask);
    mData = *tmp ^ *((unsigned long*) &mask);
    fvalue = *((double*)tmp);
    //mData = *tmp;
    bool dev = computeDeviation(fptr, fvalue, gptr, gvalue, DOUBLE);
    if (dev) { DEV_STR++; }

    // store value
    bool notInMemory = exe.getMem()->store(fptr, mData, DOUBLE);
    insideCall = 0;
}

void __STORE_PTR(unsigned long fptr, unsigned long fvalue,
                unsigned long gptr, unsigned long gvalue, unsigned char* mask)
{
    //printf("ST PTR\n");
    insideCall = 1;
    //printf("ptr MASK = %p\n", mask);
    fvalue = ( (unsigned long) fvalue) ^ ( (unsigned long) mask );
    bool dev = computeDeviation(fptr, fvalue, gptr, gvalue, PTR);
    if (dev) { DEV_STR++; }

    // store value
    bool notInMemory = exe.getMem()->store(fptr, fvalue, PTR);
    insideCall = 0;
}

void __STORE_VECTOR(unsigned long fptr, unsigned long gptr, unsigned int nElms, int type, double mask)
{
    assert(type == DOUBLE && "TYPE IS NOT DOUBLE IN __STORE_VECTOR\n");
    for (int i=0; i < nElms; i++) {
        double fvalue = ((double*)fptr)[i];
        double gvalue = ((double*)fptr)[i];
        __STORE_DOUBLE(fptr, fvalue, gptr, gvalue, i==0 ? mask : 0); // inject only first elem for now
    }
}

//=======================   Load   ===========================
unsigned long __LOAD_INT(unsigned long fptr, unsigned long gptr,
                unsigned long gvalue, unsigned long mask, int nBytes)
{
    //printf("LD INT\n");
    insideCall = 1;
    char mType;
    unsigned long mData;
    bool inMemory;
    bool dev;

    if (nBytes == 1) {
        inMemory = exe.getMem()->load(fptr, &mData, INT8);
        if (mask != 0) { shrinkMask(&mask, INT8); }
        mData ^= mask;
        dev = computeDeviation(fptr, mData, gptr, gvalue, INT8);
        if (dev) { DEV_LD++; }
    }
    else if (nBytes == 2) {
        inMemory = exe.getMem()->load(fptr, &mData, INT16);
        if (mask != 0) { shrinkMask(&mask, INT16); }
        mData ^= mask;
        dev = computeDeviation(fptr, mData, gptr, gvalue, INT16);
        if (dev) { DEV_LD++; }
    }
    else if (nBytes == 4) {
        inMemory = exe.getMem()->load(fptr, &mData, INT32);
        if (mask != 0) { shrinkMask(&mask, INT32); }
        mData ^= mask;
        dev = computeDeviation(fptr, mData, gptr, gvalue, INT32);
        if (dev) { DEV_LD++; }
    }
    else if (nBytes == 8) {
        inMemory = exe.getMem()->load(fptr, &mData, INT64);
        mData ^= mask;
        dev = computeDeviation(fptr, mData, gptr, gvalue, INT64);
        if (dev) { DEV_LD++; }
    }
    else {
        printf("ERROR: Encounted an integer load of more than 8 bytes!!!\n");
    }
    insideCall = 0;
    return mData;
}

float __LOAD_FLOAT(unsigned long fptr, unsigned long gptr, float gvalue, float mask)
{
    insideCall = 1;
    char mType;
    unsigned long mData;
    bool dev;
    bool inMemory = exe.getMem()->load(fptr, &mData, FLOAT);
    mData ^= *(reinterpret_cast<unsigned int*>(&mask)); // inject fault if mask is non-zero
    float fvalue = *((float*)(&mData));
    dev = computeDeviation(fptr, fvalue, gptr, gvalue, FLOAT);
    if (dev) { DEV_LD++; }
    insideCall = 0;
    return fvalue;
}

double __LOAD_DOUBLE(unsigned long fptr, unsigned long gptr, double gvalue, double mask)
{
    insideCall = 1;
    char mType;
    unsigned long mData;
    bool dev;
    bool inMemory = exe.getMem()->load(fptr, &mData, DOUBLE);
    mData ^= *(reinterpret_cast<unsigned long*>(&mask)); // inject fault if mask is non-zero
    double fvalue = *((double*)(&mData));
    dev = computeDeviation(fptr, fvalue, gptr, gvalue, DOUBLE);
    if (dev) { DEV_LD++; }
    insideCall = 0;
    return fvalue;
}

unsigned long __LOAD_PTR(unsigned long fptr, unsigned long gptr,
                unsigned long gvalue, unsigned char* mask)
{
    insideCall = 1;
    char mType;
    unsigned long mData;
    bool dev;
    bool inMemory = exe.getMem()->load(fptr, &mData, PTR);
    
    mData ^= (unsigned long) mask; // injects fault if mask is non-zero
    dev = computeDeviation(fptr, mData, gptr, gvalue, PTR);
    if (dev) { DEV_LD++; }
    insideCall = 0;
    return mData;
}

double __LOAD_VECTOR_ELEMENT(unsigned long fptr, unsigned long gptr, unsigned int elm, int type, double mask)
{
    assert(type == DOUBLE && "TYPE IS NOT DOUBLE IN __LOAD_VECTOR_ELEMENT\n");
    double gvalue = ((double*) gptr)[elm];
    return __LOAD_DOUBLE(fptr, gptr, gvalue, mask);
}

void shrinkMask(unsigned long* mask, char TYPE)
{
    int shift = floor(log2(*mask)) / (pow(2,TYPE)*8);
    *mask = *mask >> (shift*8);
}
