#include "Execution.h"

Execution::Execution()
{
    DISP_I = nL = nS = npL = npS = 0;
    exeGold = true;
    outfile = NULL;
}


void Execution::dumpMemory(const char* fname)
{
/*    FILE* outfile = fopen(fname, "w");
    for (auto k : *(mem.getMem())) {
        unsigned char* ptr = (unsigned char*) k.first;
        unsigned char* data = (unsigned char*) &(k.second->data);
        char type = k.second->type;
        bool diverged = k.second->dev;
        int nBytes = 0;
        
        if (type == INT8)
            nBytes = 1;
        else if (type == INT16)
            nBytes = 2;
        else if (type == INT32 || type == FLOAT)
            nBytes = 4;
        else if (type == INT64 || type == DOUBLE || type == PTR)
            nBytes = 8;
        else
            printf("ERROR: Unable to dump data of unknown type!\n");
        
        // write byte by byte
        for (int i = 0; i < nBytes; i++) {
            fprintf(outfile,"%p %u %s\n", (void*) ptr, *data, diverged ? "y":"n");
            ptr++; data++;
        }
    }
    fclose(outfile);
*/
}

void Execution::printStats(long ld, long st)
{
    if (outfile == NULL) {
        int rank = 0;
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        if (rank ==0) {
            printf("SDCPROP: Logging corruption to disk\n");
        }
        std::ostringstream oss;
        oss << "log." << rank << ".txt";
        outfile = fopen(oss.str().c_str(), "a");
    }
    //printf("Wringing to file on disk\n");
    mem.printStats(outfile, ld, st);
    //printf("Done to file on disk\n");
    fclose(outfile);
    outfile = NULL;
    //printf("---------------------------------\n");
    //mem.printStackAllocStats();
}


int Execution::logStackAlloc(unsigned char* fptr, unsigned char* gptr, unsigned int size, int type, char* fname, int line)
{
    mem.logRelateStackAlloc(fptr, gptr, size, NULL, fname, line, type);
    return 0;
}
