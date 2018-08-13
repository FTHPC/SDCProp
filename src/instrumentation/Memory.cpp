#include "Memory.h"
#include <mutex>
std::mutex alloc_mutex;

//ld, st overlap classifications
const char Memory::contained = 0;
const char Memory::after = 1;
const char Memory::before = 2;
const char Memory::both = 3;
const char Memory::outside = 4;

// operations on memory
bool Memory::load(unsigned long address, unsigned long* data, char type)
{
    int nBytes = BYTES[type];
    char loc = existsInFaulty(address, nBytes);
   *data = 0; 
    if (loc == Memory::contained) {
        memcpy(data, (void*) address, nBytes); // data = *address;
        //printf("%p = ld %p\n", *data, address);
        insideNum++;
    }
    else if (loc == Memory::outside) {
        //printf("LOAD\n");
        // load from memory
        unsigned char* ptr = (unsigned char*) data;
        for (int i=0; i<nBytes; i++) {
            if (memory.find(address+i) != memory.end())
            {
                //printf("Found in hash table\n");
                *(ptr + i) = memory[address+i];
            }
            else if (address >= (unsigned long)&etext && address <= (unsigned long)&end){
                //printf("Found inside text-data segments\n");
                memcpy(data, (void*) address, nBytes);
            }
            else
                *(ptr + i) = 0; // not in mem hash table
            unsigned char junk  = *((unsigned char*) address+i);
        }
        outsideNum++;
        //memcpy(data, (void*) address, nBytes); // data = *address;
        //printf("%p = ld %p\n", *data, address);
    //printStats();
    //printf("MEM::LD\n");
    //exit(1);
    }
    else if (loc == Memory::before) {
        printf("MEM::before\n");
        unsigned char* ptr = (unsigned char*) data;
        for (int i=0; i<nBytes; i++) {
            if (address >= (unsigned long)&etext && address <= (unsigned long)&end){
                //printf("Found inside text-data segments\n");
                memcpy(data, (void*) address, nBytes);
            }
            else if (memory.find(address+i) != memory.end())
            {
                //printf("Found in hash table\n");
                *(ptr + i) = memory[address+i];
            }
            else {
                // rest of load is in an allocation
                memcpy(data+i, (void*) (address+i), nBytes-i);
                i = nBytes; // terminate loop
                continue;
            }
            unsigned char junk  = *((unsigned char*) address+i);
        }
        beforeNum++;
    }
    else if (loc == Memory::after) {
        printf("MEM::after\n");
        unsigned char* ptr = (unsigned char*) data;
        for (int i=nBytes-1; i>=0; i--) {
            if (address >= (unsigned long)&etext && address <= (unsigned long)&end){
                //printf("Found inside text-data segments\n");
                memcpy(data, (void*) address, nBytes);
            }
            else if (memory.find(address+i) != memory.end())
            {
                //printf("Found in hash table\n");
                *(ptr + i) = memory[address+i];
            }
            else {
                // rest of load is in an allocation
                memcpy(data, (void*) address, i);
                i = -1; // terminate loop
                continue;
            }
            unsigned char junk  = *((unsigned char*) address+i);
        }
        afterNum++;
    }
    else if (loc == Memory::both) {
        printf("MEM::both\n");
        unsigned char* ptr = (unsigned char*) data;
        for (int i=0; i<nBytes; i++) {
            if (address >= (unsigned long)&etext && address <= (unsigned long)&end){
                //printf("Found inside text-data segments\n");
                memcpy(data, (void*) address, nBytes);
            }
            else if (memory.find(address+i) != memory.end())
            {
                //printf("Found in hash table\n");
                *(ptr + i) = memory[address+i];
            }
            else {
                // rest of load is in an allocation
                memcpy(data+i, (void*) (address+i), 1);
            }
            unsigned char junk  = *((unsigned char*) address+i);
        }
        bothNum++;
    }
    return true;
}

bool Memory::store(unsigned long address, unsigned long data, char type)
{
    int nBytes = BYTES[type];
    char loc = existsInFaulty(address, nBytes);

    if (loc == Memory::contained) {
      //  printf("st %p, (%p)\n", data, address);
        memcpy((void*) address, &data, nBytes); //*address = data;
        insideNum++;
    }
    else if (loc == Memory::outside) {
        // store into memory
        //printf("STORE\n");
        unsigned char* ptr = (unsigned char*) &data;
        for (int i=0; i<nBytes; i++) {
            memory[address+i] = *(ptr + i);
            unsigned char junk  = *((unsigned char*) address+i);
        }
        outsideNum++;
        //memcpy((void*) address, &data, nBytes); //*address = data;
        //printf("st %p, (%p)\n", data, address);
    //printStats();
    //printf("MEM::ST\n");
    //exit(1);
    }
    else if (loc == Memory::before) {
        printf("MEM::before\n");
        unsigned char* ptr = (unsigned char*) &data;
        for (int i=0; i<nBytes; i++) {
            /*
 * if (address >= (unsigned long)&etext && address <= (unsigned long)&end){
                //printf("Found inside text-data segments\n");
                memcpy(data, (void*) address, nBytes);
            }
            else*/
            if (memory.find(address+i) != memory.end())
            {
                memory[address+i] = *(ptr+i);
            }
            else {
                memcpy((void*) (address+i), ptr+i, nBytes-i);
                i = nBytes; // terminate loop
                continue;
            }
            unsigned char junk  = *((unsigned char*) address+i);
        }
        beforeNum++;
    }
    else if (loc == Memory::after) {
        printf("MEM::after\n");
        unsigned char* ptr = (unsigned char*) &data;
        for (int i=nBytes-1; i>=0; i--) {
            /*if (address >= (unsigned long)&etext && address <= (unsigned long)&end){
                //printf("Found inside text-data segments\n");
                memcpy(data, (void*) address, nBytes);
            }
            else*/
            if (memory.find(address+i) != memory.end())
            {
                memory[address+i] = *(ptr+i);
            }
            else {
                // rest of load is in an allocation
                memcpy((void*) address, &data,  i);
                i = -1; // terminate loop
                continue;
            }
            unsigned char junk  = *((unsigned char*) address+i);
        }
        afterNum++;
    }
    else if (loc == Memory::both) {
        printf("MEM::both\n");
        unsigned char* ptr = (unsigned char*) &data;
        for (int i=0; i<nBytes; i++) {
            /*
 * if (address >= (unsigned long)&etext && address <= (unsigned long)&end){
                //printf("Found inside text-data segments\n");
                memcpy(data, (void*) address, nBytes);
            }
            else*/
            if (memory.find(address+i) != memory.end())
            {
                memory[address+i] = *(ptr+i);
            }
            else {
                // rest of load is in an allocation
                memcpy((void*) (address+i), ptr+i, 1);
            }
            unsigned char junk  = *((unsigned char*) address+i);
        }
        bothNum++;
    }
    return true;
}

char Memory::existsInFaulty(unsigned long address, int elmSize)
{
    //printf("MA of %d bytes\n", elmSize);
    for (auto I : {stackAllocs, allocs}) {
        for (auto alloc : I) {
            unsigned long fbase = (unsigned long)alloc->fbase;
            size_t size = alloc->size;

            if (address >= fbase && address <= fbase+size-elmSize) {
               // printf("Memory::contained(%p)\n",(void*)address);
                return Memory::contained;
            }
            //TODO: handle part in part out case
            else if (address >= fbase && address+elmSize > fbase+size
                        && address < fbase + size)
                return Memory::after;
            else if (address < fbase && address+elmSize <= fbase+size
                        && address+elmSize > fbase)
                return Memory::before;
            else if (address < fbase && address+elmSize > fbase+size)
                return Memory::both;
        }
    }
    //printf("Memoryaccess is OUTSIDE(%p)\n", (void*)address);
    return Memory::outside;
}

unsigned long Memory::getFaulty(unsigned long addr)
{
    for (auto a : allocs) {
        if ((unsigned long) a->gbase == addr) {
            return (unsigned long)a->fbase;
        }
    }
    return addr;
}


// adds allocations into memory
void Memory::logMemoryGlobal(unsigned char* fptr, unsigned char* gptr, unsigned long size)
{
assert(fptr != NULL && "fptr is NULL\n");
assert(gptr != NULL && "gptr is NULL\n");
    // if gbl mem address exists in allocs then do not add the alloc
    for (auto alloc : allocs) {
        if (alloc->fbase == fptr && alloc->gbase == gptr) {
            return;
        }
    }
    MemAlloc* a = new MemAlloc();
    assert(a != NULL && "MemAlloc is NULL in alloc logging.");
    a->gbase = gptr; a->fbase = fptr;
    a->size = size; a->trace = NULL; a->id = -1;
    //printf("Global Log fptr=%p, gptr%p, z=%lu, trace=%p\n", fptr, gptr, size, a->trace);
    allocs.push_back(a);
}

void Memory::logAlloc(void* fptr, void* gptr, unsigned long size, void* trace, int type, char* file, int line, int id)
{
    MemAlloc* a = new MemAlloc();
    assert(a != NULL && "MemAlloc is NULL in alloc logging.");
    a->gbase = gptr; a->fbase = fptr;
    a->size = size; a->trace = trace; a->id = type;
    //printf("Malloc fptr=%p, gptr%p, z=%lu, trace=%p\n", fptr, gptr, size, trace);
assert(fptr != NULL && "fptr is NULL\n");
assert(gptr != NULL && "gptr is NULL\n");
//        alloc_mutex.lock();
    allocs.push_back(a);
    if (addr2line.find(fptr) == addr2line.end()) {
        addr2line[fptr] = std::pair<char*, int>(file, line);
    }
    else {
        printf("MEM: heap fptr already exists in addr2line\n");
    }
//        alloc_mutex.unlock();
}

void Memory::logRealloc(void* fptr, void* gptr, void* old_fptr, void* old_gptr, unsigned long size, void* trace, int type, char* file, int line, int id)
{
    //printf("Realloc fptr=%p, gptr%p, z=%lu, trace=%p\n", fptr, gptr, size, trace);
assert(fptr != NULL && "fptr is NULL in realloc\n");
assert(gptr != NULL && "gptr is NULL in realloc\n");

    int i=0;
    for (auto alloc = allocs.begin(); alloc != allocs.end(); alloc++) {
        if ((*alloc)->fbase == fptr) {
            if (fptr == old_fptr) { // increase alloc size
                (*alloc)->size = size;
            }
            else /* realloc moved the memory*/ {
                //update addr2line
                auto tmp = addr2line[(*alloc)->fbase];
                addr2line[fptr] = std::pair<char*, int>(tmp.first, tmp.second);
                //addr2line[fptr] = addr2line[(*alloc)->fbase];
                addr2line.erase((*alloc)->fbase);
                
                // update allocs
                MemAlloc* a = new MemAlloc();
                assert(a != NULL && "MemAlloc is NULL in alloc logging.");
                a->gbase = gptr; a->fbase = fptr;
                a->size = size; a->trace = trace; a->id = type;
                delete allocs[i];
                allocs[i] = a;
                continue;
            }
        }
        i++;
    }
}

int Memory::freeAlloc(void* ptr)
{
    //for (auto& A : {allocs, callAllocs}) {
    int i=0;
    for (auto alloc = allocs.begin(); alloc != allocs.end(); alloc++) {
        if ((*alloc)->fbase == NULL) {
            //printf("ERASE: Malloc fptr=%p, gptr%p, z=%lu, trace=%p\n",
            //        (*alloc)->fbase, (*alloc)->gbase, (*alloc)->size, (*alloc)->trace);
            delete allocs[i];
            allocs.erase(allocs.begin()+i);
            continue;
        }
        assert((*alloc)->fbase != NULL && "free(malloc= NULL)\n");
        if ((*alloc)->fbase == ptr) {
            //printf("ERASE: Malloc fptr=%p, gptr%p, z=%lu, trace=%p\n",
            //        (*alloc)->fbase, (*alloc)->gbase, (*alloc)->size, (*alloc)->trace);
            if (addr2line.find(ptr) != addr2line.end()) {
                addr2line.erase(ptr);
                //addr2line.erase(pos);
            }
            else {
                printf("MEM: Can't remove heap ptr %p from addr2line (doesn't exist)\n", ptr);
            }
            delete allocs[i];
            allocs.erase(allocs.begin()+i);
            return 1;
        }
        i++;
    }
    return 0;
}

void Memory::logRelateStackAlloc(void* fptr, void* gptr, unsigned long size, void* trace, char* fname, int line, int id)
{
    nAllocsPerFunc.top() += 1;
    //printf("SA: fp=%p, gp=%p, z=%lu trace=%p\n", fptr, gptr, size, trace);
    MemAlloc* a = new MemAlloc();
    assert(a != NULL && "MemAlloc is NULL in stack logging.");
    a->gbase = gptr; a->fbase = fptr;
    a->size = size; a->trace = trace;
    a->id = id;
    stackAllocs.push_back(a);
    if (addr2line.find(fptr) == addr2line.end()) {
        addr2line[fptr] = std::pair<char*, int>(fname, line);
    }
    else {
        //printf("MEM: stack fptr already exists in addr2line\n");
    }
}

void Memory::removeAlloca(int num)
{
    num = nAllocsPerFunc.top();
    nAllocsPerFunc.pop();
    assert(stackAllocs.size() >= num && "Removing too many stack allocations!\n");
    //printf("Removing %d stack allocs from log of %d\n", num, stackAllocs.size());
    for (auto i = stackAllocs.size()-num; i < stackAllocs.size(); i++) {
        //printf("RM SA: fp=%p, gp=%p, size=%d\n", stackAllocs[i]->fbase, stackAllocs[i]->gbase, stackAllocs[i]->size);
        if (addr2line.find(stackAllocs[i]->fbase) != addr2line.end()) {
            addr2line.erase(stackAllocs[i]->fbase);
            //addr2line.erase(pos);
        }
        else {
            //printf("MEM: Can't remove ptr %p from addr2line (doesn't exist)\n", stackAllocs[i]->fbase);
        }
        delete stackAllocs[i];
    }
    stackAllocs.erase(stackAllocs.end()-num, stackAllocs.end());
}



void Memory::relateLoggedMallocs(char single)
{
    printf("Inside void Memory::relateLoggedMallocs(char single) \n");
    exit(1);
    /*
    if (single == 1) {
        //add each logged malloc to allocs, but gptr=fptr
        for (auto a : callAllocs) {
            logAlloc(a->gbase, a->gbase, a->size, a->trace, a->id);
        }
    }
    else {
        int half = callAllocs.size()/2;
        if (half >- 0)
            //printf("ptr=%p, z=%d\n", callAllocs[0]->fbase, callAllocs.size());
        assert(callAllocs.size() %2 == 0 && "Odd number of mallocs logged when call is duplicated\n");
        for (int i=0; i < half; i++) {
            auto gold = callAllocs[i];
            auto faulty = callAllocs[half + i];
            assert(gold->trace == faulty->trace && "Different traces for mallocs logged when call is duplicated\n");
            logAlloc(faulty->gbase, gold->gbase, gold->size, gold->trace, gold->id);
        }
    }callAllocs.erase(callAllocs.begin(), callAllocs.end());
*/
}

int Memory::getStackAllocID(void* ptr)
{
    return -1;
}



// data collection output
void Memory::copyMem2Faulty(int mode)
{
    //TODO: add options for mode
    // deep - copy all mallocs
    // shallow - copy all since last copy
    for (auto alloc : allocs) {
        //printf("memcpy(%p, %p, %lu);\n", alloc->fbase, alloc->gbase, alloc->size);
        memcpy(alloc->fbase, alloc->gbase, alloc->size);
    }
}
//#define DATA_TOL 1e-10
void Memory::printStats(FILE* outfile, long ld, long st)
{
    assert(outfile != NULL && "outfile is NULL in Memory::printStats\n");
    // mallocs, and stackallocs
    int i=0;
    //for (auto I : {allocs}) {
    fprintf(outfile, "\n###START###\n");
    fprintf(outfile, "DEV_LD = %lu\nDEV_ST = %lu\n", ld, st);
    fprintf(outfile, "MAX_DEV = %g\n", maxValueDev);
    fprintf(outfile, "IN = %lu\nOUT = %lu\nBEFORE = %lu\nAFTER = %lu\nBOTH=%lu\n",
            insideNum, outsideNum, beforeNum, afterNum, bothNum);

    for (auto I : {allocs, stackAllocs}) {
        for (auto alloc : I) {
            assert(alloc->fbase != NULL && "printStats: fptr= NULL)\n");
            unsigned char* gbase = (unsigned char*) alloc->gbase;
            unsigned char* fbase = (unsigned char*) alloc->fbase;
            size_t size = alloc->size;
            void* trace = alloc->trace;
            int nElmsCrptd = 0;
            double maxErr = 0.0; 
            double norm = 0.0;
            
            // compute deviation based on type
            if (alloc->id == INT16) {
                unsigned short* G = (unsigned short*) gbase;
                unsigned short* F = (unsigned short*) fbase;
                size = size/BYTES[INT16];
                for (auto i=0; i<size; i++) {
                    auto diff = G[i] - F[i];
                    norm += diff*diff;
                    if (diff != 0)
                        nElmsCrptd++;
                    if ( abs(diff) > maxErr)
                        maxErr = abs(diff);
                }
            }
            else if (alloc->id == INT32) {
                unsigned int* G = (unsigned int*) gbase;
                unsigned int* F = (unsigned int*) fbase;
                size = size/BYTES[INT32];
                for (auto i=0; i<size; i++) {
                    auto diff = G[i] - F[i];
                    norm += diff*diff;
                    if (diff != 0)
                        nElmsCrptd++;
                    if ( abs(diff) > maxErr)
                        maxErr = abs(diff);
                }
            }
            else if (alloc->id == INT64 || alloc->id == PTR) {
                unsigned long* G = (unsigned long*) gbase;
                unsigned long* F = (unsigned long*) fbase;
                size = size/BYTES[INT64];
                for (auto i=0; i<size; i++) {
                    auto diff = G[i] - F[i];
                    norm += diff*diff;
                    if (diff != 0)
                        nElmsCrptd++;
                    if ( abs(diff) > maxErr)
                        maxErr = abs(diff);
                }
            }
            else if (alloc->id == FLOAT) {
                float* G = (float*) gbase;
                float* F = (float*) fbase;
                size = size/BYTES[FLOAT];
                for (auto i=0; i<size; i++) {
                    auto diff = G[i] - F[i];
                    norm += diff*diff;
                    if ( (double) fabs(diff) > DATA_TOL)
                        nElmsCrptd++;
                    if ( (double) fabs(diff) > maxErr)
                        maxErr = fabs(diff);
                }
            }
            else if (alloc->id == DOUBLE) {
                double* G = (double*) gbase;
                double* F = (double*) fbase;
                size = size/BYTES[DOUBLE];
                for (auto i=0; i<size; i++) {
                    auto diff = G[i] - F[i];
                    norm += diff*diff;
                    if ( (double) fabs(diff) > DATA_TOL)
                        nElmsCrptd++;
                    if ( (double) fabs(diff) > maxErr)
                        maxErr = fabs(diff);
                }
            }
            else {
                for (auto i=0; i<size; i++) {
                    auto diff = gbase[i] - fbase[i];
                    norm += diff*diff;
                    if (diff != 0)
                        nElmsCrptd++;
                    if ( abs(diff) > maxErr)
                        maxErr = abs(diff);
                }
            }
            norm = sqrt(norm);
            if (addr2line.find(fbase) != addr2line.end()) {
                //fprintf(outfile, "Corruption of %d bytes in alloc of %lu bytes at %p from text %s:%d: %g%%\n",
                //    nElmsCrptd, size, fbase, addr2line[fbase].first, addr2line[fbase].second, ((double)nElmsCrptd)/size*100);
                // Crpt% size baseAddr text maxErr
                // mapping to source file exists
            
                fprintf(outfile, "%d %d %lu %p %s:%d %g %g\n",
                    alloc->id, nElmsCrptd, size, fbase, addr2line[fbase].first, addr2line[fbase].second, maxErr, norm);
            }
            else {
                //fprintf(outfile, "Corruption of %d bytes in alloc of %lu bytes at %p from text %p: %g%%\n",
                //    nElmsCrptd, size, fbase, trace, ((double)nElmsCrptd)/size*100);
                fprintf(outfile, "%d %d %lu %p %p:0 %g %g\n",
                    alloc->id, nElmsCrptd, size, fbase, trace, maxErr, norm);
            }
        }
        fprintf(outfile, "\n------------------------------\n");
    }
    fprintf(outfile, "Bytes written outside allocated regions: %lu\n", memory.size());
    fprintf(outfile, "\n###END###\n");
}

void Memory::getCorruption(const void* buf, const int count, const int size,
    void** data, int** idxs, int* num)
{
    //TODO: pass type to this function or assume that we mark it byte by byte
    *num = 0;
    *idxs = (int*)malloc(count * sizeof(int));
    *data = malloc(count *size);
    //walk down addresses and pull the deviation value
    for (auto i = 0; i < count; i++) {
        unsigned long buf_addr = ((unsigned long)buf) + i*size;
        unsigned long data_addr = (unsigned long)(*data) + i*size;
        bool dev; unsigned long tmp_data; char type;
        
        load(buf_addr, &tmp_data, type);
        dev = false; //TODO: FIX HERE <_----------
        memcpy((void*) data_addr,(void*) &tmp_data, size);
        if (dev) {
            (*idxs)[*num] = i;
            *num += 1;
        }
    }
}


void Memory::setCorruption(const void* buf, const int count, const char type,
            const void* data, const int* idxs, const int num)
{
    int pos = 0;
    //walk down addresses and store the value and deviation
    for (auto i = 0; i < count; i++) {
        unsigned long buf_addr = ((unsigned long)buf) + i*BYTES[type];
        unsigned long data_addr = (unsigned long)(data) + i*BYTES[type];
        bool dev = idxs[pos] == i;
        unsigned long tmp_data;
        
        memcpy((void*) &tmp_data,(void*) data_addr, BYTES[type]);
        store(buf_addr, tmp_data, type); 
        if (dev) {
            pos++;
        }
    }
}
