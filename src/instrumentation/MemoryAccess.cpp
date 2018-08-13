#include "MemoryAccess.h"
#include <stdlib.h>

#define COMPUTE_DEV 1

double maxValueDev = 0;
MemoryAccess::MemoryAccess(unsigned long address, bool load, unsigned long data, int type)
{
    this->address = address;
    this->load = load;
    this->data = data;
    this->type = type;

    this->ptrDev = 0;
    this->dataDev = 0;

    //printf("MA: %p, %s, %p, %s\n", address, load ? "load" : "store", data, TYPE_STR[type+1]);
}

bool MemoryAccess::computeDeviation(MemoryAccess* ma, unsigned long stackOffset)
{
    if (ma == NULL) {
        return false;
    }
    // compute deviation in the address for the Mem Acc
    if (address > ma->address)
        ptrDev = address - ma->address;// - stackOffset; 
    else
        ptrDev = ma->address - address;// - stackOffset;
    if (ptrDev != 0)
        ptrDev -= stackOffset;
    if (ptrDev != 0)
        ptrDev = ptrDev + stackOffset - 176;
    
    if (type == UNKNOWN) {
        /*printf("WARNING: MA::computeDeviation trying to "
            "compute deviation of unknown type (%s). "
            "Converting to Gold type (%s)\n", 
            TYPE_STR[type+1], TYPE_STR[ma->type+1]);
        */
        type = ma->type;
    }
    // computing the deviation in the value depends on the data type
    if (type >= INT8 && type <= INT64) {
        if (data > ma->data)
            dataDev = (double)(data - ma->data);
        else
            dataDev = (double)(ma->data - data);
    }
    else if (type == FLOAT) {
        float* ptr = (float*)&data;
        float data_f = *ptr;
        ptr = (float*)&(ma->data);
        float maData_f = *ptr;
        
        dataDev = (double) fabs(data_f - maData_f);
    }
    else if (type == DOUBLE) {
        double* ptr = (double*)&data;
        double data_f = *ptr;
        ptr = (double*)&(ma->data);
        double maData_f = *ptr;
        
        dataDev = fabs(data_f - maData_f);
    }
    else if (type == PTR) {
        if (data > ma->data)
            dataDev = (double)(data - ma->data);
        else
            dataDev = (double)(ma->data - data);
    }
    else {
        /*printf("ERROR: MemoryAccess::computeDeviation trying to "
            "compute deviation of unknown type (%s). Gold type "
            "(%s)\n", TYPE_STR[type+1], TYPE_STR[ma->type+1]);
        */
    }

    if (getDeviated()) {
        /*printf("WARNING: %s deviation in memory access"
            "(ptr = %lu) (value = %g) (offset = %lu)\n",
            load ? "Load" : "Store", ptrDev, dataDev, stackOffset);
        
        printf("address = %p, ma->address = %p\n", (void*)address, (void*)ma->address);
        printf("data = %lu, ma->data = %lu\n", data, ma->data);
       */
        //exit(1);
    /*
        if (type >= INT8 && type <= INT64)
            printf("I: data = %lu, ma->data = %lu\n", data, ma->data);
        else if (type == FLOAT)
            printf("F: data = %g, ma->data = %g\n", data_f, maData_f);
        else if (type == DOUBLE)
            printf("D: data = %g, ma->data = %g\n", data_f, maData_f);
        else if (type == PTR)
            printf("P: data = %p, ma->data = %p\n", (void*)data, (void*)ma->data);
    */ 
    }
    return getDeviated();
}

bool MemoryAccess::getDeviated()
{
    return (ptrDev != 0) || (dataDev > 1e-13);
}

double DATA_TOL = 1e-12;
//#define DATA_TOL = 1e-10;
bool computeDeviation(unsigned long fptr, float fvalue,
                    unsigned long gptr, float gvalue, char type)
{
    return computeDeviation(fptr, (unsigned long) (*((unsigned int*)&fvalue)),
                gptr, (unsigned long)(*((unsigned int*)&gvalue)), type);
}

bool computeDeviation(unsigned long fptr, double fvalue,
                    unsigned long gptr, double gvalue, char type)
{
    return computeDeviation(fptr, *((unsigned long*)&fvalue),
                gptr,  *((unsigned long*)&gvalue), type);
}

bool computeDeviation(unsigned long fptr, unsigned long fvalue,
                    unsigned long gptr, unsigned long gvalue, char type)
{
    unsigned long ptrDev = 0;
    double dataDev = 0.0;
    bool ret = false;
#ifdef COMPUTE_DEV
    // compute deviation in the address
    if (fptr > gptr)
        ptrDev = fptr - gptr;
    else
        ptrDev = gptr - fptr;
    
    if (type == UNKNOWN) {
        printf("WARNING: computeDeviation trying to "
            "compute deviation of unknown type");
            // (%s). Converting to Gold type (%s)\n", 
            //TYPE_STR[fMA->type+1], TYPE_STR[gMA->type+1]);
    }
    
    // computing the deviation in the value depends on the data type
    if (type >= INT8 && type <= INT64) {
        if (fvalue > gvalue)
            dataDev = (double)(fvalue - gvalue);
        else
            dataDev = (double)(gvalue - fvalue);
    }
    else if (type == FLOAT) {
        float data_f = *((float*)&(fvalue));
        float data_g = *((float*)&(gvalue));
        dataDev = (double) fabs(data_f - data_g);
    }
    else if (type == DOUBLE) {
        double data_f = *((double*)&(fvalue));
        double data_g = *((double*)&(gvalue));
        dataDev = fabs(data_f - data_g);
    }
    else if (type == PTR) {
        if (fvalue > gvalue)
            dataDev = (double)(fvalue - gvalue);
        else
            dataDev = (double)(gvalue - fvalue);
        //dataDev = (double) abs(fvalue - gvalue);
    }
    else {
        /*printf("ERROR: MemoryAccess::computeDeviation trying to "
            "compute deviation of unknown type (%s). Gold type "
            "(%s)\n", TYPE_STR[type+1], TYPE_STR[ma->type+1]);
        */
    }
    
    if (dataDev > DATA_TOL) {
    //if ((ptrDev != 0) || (dataDev > DATA_TOL)) {
        ret = true;
        //printf("dd=%g, mdd=%g\n", dataDev, maxValueDev);
        if ( (dataDev > maxValueDev) && (type == FLOAT || type == DOUBLE)){
            maxValueDev = dataDev;
            printf("MAX VALUE DEV = %g\n", maxValueDev);
        }
        /*
         printf("WARNING: deviation in memory access (ptr = %lu)"
                " (value = %g)\nfptr = %p, gptr = %p\n"
                "fvalue = %lu, gvalue = %lu\n",ptrDev, dataDev,
                (void*)fptr, (void*)gptr, fvalue, gvalue);
       */
    }
    else {
        ret = false;
    }
#endif
    return ret;
}
