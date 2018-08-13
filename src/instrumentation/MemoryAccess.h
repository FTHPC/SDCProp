#ifndef MEMORYACCESS_H
#define MEMORYACCESS_H
/*
struct MemoryAccess
{
    unsigned long address;
    unsigned long data;
    int nBytes;
    int type;
    bool load;

};
*/
#include <stdio.h>
#include <math.h>
#include "Types.h"

extern double maxValueDev;
extern double DATA_TOL;
struct MemoryAccess_t {
    unsigned long address;
    unsigned long data;
    char type;
    bool store;
    bool deviation;
};

bool computeDeviation(unsigned long fptr, float fvalue,
                    unsigned long gptr, float gvalue, char type);

bool computeDeviation(unsigned long fptr, double fvalue,
                    unsigned long gptr, double gvalue, char type);

bool computeDeviation(unsigned long fptr, unsigned long fvalue,
                    unsigned long gptr, unsigned long gvalue, char type);

/*****************************************************************************/
class MemoryAccess
{
    public:
        MemoryAccess(unsigned long address, bool load, unsigned long data, int type);
        bool computeDeviation(MemoryAccess* ma, unsigned long stackOffset);
        bool getDeviated();
 
        unsigned long getAddress() { return address; }
        bool isLoad() { return load; }
        bool isStore() { return !load;}
        unsigned long getData() { return data; }
        int getType() { return type; }
        double getDataDeviation() { return dataDev; }
        unsigned long getAddressDeviation() { return ptrDev; }
    private:
        unsigned long address;
        bool load;
        unsigned long data;
        int type;
        double dataDev;
        unsigned long ptrDev;        
};
#endif
