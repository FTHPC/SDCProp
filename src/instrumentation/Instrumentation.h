#ifndef INSTRUMENTATION_H
#define INSTRUMENTATION_H

//#include "Execution.h"
//#include "Memory.h"
#include <stdio.h>
#include "Types.h"
#include <execinfo.h>
#include <unistd.h>
#include <signal.h>

#ifdef __cplusplus
extern "C" {
#endif
extern void* argBuf;
extern int logCallMallocs;

void setInstCount(unsigned long long (foo)());

void copyMem2Faulty(int mode);
void dumpMemory(int start, int end, const char* gold, const char* faulty);
int logMemoryAlloc(void* fptr, void* gptr, long size, void* trace, int type, char* file, int line);
int logMemoryRealloc(void* fptr, void* gptr, void* old_fptr, void* old_gptr, long size, void* trace, int type, char* file, int line);
int freeMemoryAlloc(void* ptr);
void printStats();
int inInstrumentationCall();
void getCorruption(const void* buf, const int count, const int size, void** data,
    int** idxs, int* num);
void setCorruption(const void* buf, const int count, const char type, const void* data,
    const int* idxs, const int num);
void shrinkMask(unsigned long* mask, char type);
void logMallocInCall(void* ptr, unsigned long size, void* trace);

void __DUMMY_FREE(unsigned char* ptr);
int __LOG_ALLOCA(unsigned char* ptr, unsigned long num, unsigned long size);
unsigned long __GET_FAULTY_VALUE(unsigned long gaddr, unsigned long faddr, unsigned long gvalue);
/****************  New Instrumentation Functions **********/
void __LOG_AND_RELATE_GLOBAL(unsigned char* fptr, unsigned char* gptr, unsigned long size);
void __LOG_MALLOCS_IN_CALL();
void __RELATE_MALLOCS_IN_CALL(char type);
void __LOG_CMD_ARGS(int argc, char** argv);
void __REMOVE_ALLOCA(int num);
int __LOG_AND_RELATE_ALLOCA(unsigned char* fPtr, unsigned char* gPtr,
                            unsigned long num, unsigned long size, int type,
                            char* fname, int line);
void __LOG_AND_RELATE_MALLOC(unsigned char* fPtr, unsigned char* gPtr,
                            unsigned long size, int type, char* fname, int line);
void __LOG_AND_RELATE_REALLOC(unsigned char* fPtr, unsigned char* gPtr,
                            unsigned char* old_fptr, unsigned char* old_gptr,
                            unsigned long size, int type, char* fname, int line);
void __CHECK_CMP(unsigned faulty, unsigned gold, unsigned BB_ID);
void __START_FUNCTION();
//=======================   Store   ===========================
void __STORE_INT(unsigned long fptr, unsigned long fvalue,
                unsigned long gptr, unsigned long gvalue,
                int nBytes, unsigned long mask);
void __STORE_FLOAT(unsigned long fptr, float fvalue, unsigned long gptr, float gvalue, float mask);
void __STORE_DOUBLE(unsigned long fptr, double fvalue, unsigned long gptr, double gvalue, double mask);
void __STORE_PTR(unsigned long fptr, unsigned long fvalue,
                unsigned long gptr, unsigned long gvalue, unsigned char* mask);

void __STORE_VECTOR(unsigned long fptr, unsigned long gptr, unsigned int nElms, int type, double mask);
//=======================   Load   ===========================
unsigned long __LOAD_INT(unsigned long fptr, unsigned long gptr,
                unsigned long gvalue, unsigned long mask, int nBytes);
float __LOAD_FLOAT(unsigned long fptr, unsigned long gptr, float gvalue, float mask);
double __LOAD_DOUBLE(unsigned long fptr, unsigned long gptr, double gvalue, double mask);
unsigned long __LOAD_PTR(unsigned long fptr, unsigned long gptr,
                unsigned long gvalue, unsigned char* mask);
double __LOAD_VECTOR_ELEMENT(unsigned long fptr, unsigned long gptr, unsigned int elm, int type, double mask);





#ifdef __cplusplus
}
#endif
#endif
