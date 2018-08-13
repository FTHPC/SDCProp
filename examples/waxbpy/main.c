#include "FlipIt/corrupt/corrupt.h"
#include "Instrumentation.h"
#include "mpi.h"
#include "waxpby.h"
#include <stdlib.h>

#include <signal.h>

void sigHandler(int sig)
{
    printf("Receiving Sig %d\n", sig);
    printStats();
    unsigned long long t = FLIPIT_GetExecutedInstructionCount();
    int r;
    MPI_Comm_rank(MPI_COMM_WORLD, &r);
    printf("Rank %d: Total Insts executed = %llu\n", r, FLIPIT_GetExecutedInstructionCount());
    exit(sig);//dumpMemory(0,-1,"gold.txt", "faulty.txt");
    //    fflush(stdout);
    //        exit(sig);
    //        }
}

void waxpbyLogger(FILE* outfile)
{
    int r;
    MPI_Comm_rank(MPI_COMM_WORLD, &r);
    //fprintf(outfile, "\nRank %d: Injection in Iteration = %d\n", r, GLOBAL_TIMESTEP);
    unsigned long long t = FLIPIT_GetExecutedInstructionCount();
    if (t != 0)
        fprintf(outfile, "Rank %d: Total Insts executed = %llu\n", r, FLIPIT_GetExecutedInstructionCount());
}


int main(int argc, char *argv[])
{
    if (signal (SIGSEGV, sigHandler) == SIG_ERR)
        printf("Error setting segfault handler...\n");
    if (signal (SIGBUS, sigHandler) == SIG_ERR)
        printf("Error setting bus error handler...\n");
    MPI_Init(&argc, &argv);
    printf("First line\n");
    double *x, *y, *w;
    double alpha = 1.0;
    double beta= 2.0;
    int size, rank; // Number of MPI processes, My process ID
    int N = 4096;
    int i = 0;

    x = (double*) malloc(N*sizeof(double));
    y = (double*) malloc(N*sizeof(double));
    w = (double*) malloc(N*sizeof(double));
    
    for (i =0; i < N; i ++)
    {
        x[i] = i+100;
        y[i] = N-i + 250;
    } 
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    FLIPIT_Init(rank, argc, argv, atol(argv[argc-1]));
    FLIPIT_SetInjector(FLIPIT_OFF);
    FLIPIT_CountdownTimer(atol(argv[argc-1]));
    FLIPIT_SetCustomLogger(waxpbyLogger);
    
    FLIPIT_SetInjector(FLIPIT_ON);
    printStats();
    unsigned long start = FLIPIT_GetExecutedInstructionCount();
        printf("Before waxpby call %lu\n", start);
    waxpby (N, alpha, x, beta, y, w);
    unsigned long end = FLIPIT_GetExecutedInstructionCount();
    printf("DONE -- %lu\n", end);
    printf("Total insts executed: %lu\n", end - start);
    FLIPIT_SetInjector(FLIPIT_OFF);
    printStats();
    
    FLIPIT_Finalize(NULL);
    //FLIPIT_Finalize("Histo.txt");
    MPI_Finalize();
    return 0 ;
}

