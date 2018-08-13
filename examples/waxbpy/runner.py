import os
import sys
import random

nTrials = int(sys.argv[1])
seed = int (sys.argv[2])
random.seed(seed)
start = 0
if len(sys.argv) > 3:
    start = int(sys.argv[3])

step = 125
for i in range(start, nTrials, step):
    #os.system("mkdir test_" + str(i))
   
    outfile = open("fault" + str(i) + ".pbs", "w")
    outfile.write("#!/bin/bash\n"
                "#PBS -l nodes=1:ppn=16:xe\n"
                "#PBS -l walltime=1:00:00\n"
                "#PBS -q normal\n"
                "#PBS -j oe\n"
                "#PBS -A jr5\n"
                "#PBS -N WAXPBY_SDCPROP\n")

    for s in xrange(step):
        os.system("mkdir test_" + str(seed))
        outfile.write("cd $PBS_O_WORKDIR/test_" +str(seed)+"\n"
                    "aprun -n 1 /u/sciteam/jccalho2/research/sdcprop/tests/compileTest/test_waxpby  --stateFile /u/sciteam/jccalho2/research/FlipIt/.foo --numberFaulty 1 --faulty 0 " +  str(seed) + " &> test_" +str(seed) + ".txt\n")
                    #"aprun -n 1 /u/sciteam/jccalho2/research/sdcprop/tests/compileTest/test_waxpby  --stateFile /u/sciteam/jccalho2/research/FlipIt/.foo --numberFaulty 1 --faulty 0 " +  str(seed) + " &> /u/sciteam/jccalho2/research/sdcprop/tests/compilerTest/test_" + str(seed) + "/test_" +str(seed) + ".txt\n")
        seed += 1
    outfile.close()
    
    print "Launching job", i
    os.system("qsub fault" + str(i) + ".pbs")



#nTrials = int(sys.argv[1])
#seed = int (sys.argv[2])
#random.seed(seed)

#for i in range(0, nTrials):
#    os.system("mkdir test_" + str(i))
    '''
    outfile = open("fault" + str(i) + ".pbs", "w")
    outfile.write("#!/bin/bash\n"
                "#PBS -l nodes=1:ppn=16:xe\n"
                "#PBS -l walltime=0:15:00\n"
                "#PBS -q low\n"
                "#PBS -j oe\n"
                "#PBS -A jr5\n"
                "#PBS -N HPCCG_SDCPROP\n"
                "cd $PBS_O_WORKDIR/test_" +str(i)+"\n"
                "aprun -n 4  /u/sciteam/jccalho2/research/sdcprop/tests/HPCCG-1.0/test_HPCCG 10 10 10 --stateFile /u/sciteam/jccalho2/research/FlipIt/.foo --numberFaulty 1 --faulty 3 " \
                #+ str(random.randint(0,4)) + " " \
                #+ str(random.randint(0,2123940)) + " &> test_" +str(i) + ".txt\n")
                + str(random.randint(0,50000000)) + " &> test_" +str(i) + ".txt\n")
    outfile.close()
    '''
#    print "Launching job", i
    #os.system("qsub fault" + str(i) + ".pbs")
#    print "mpirun -np 1 ./test_waxpby --stateFile /home/aperson/research/FlipIt/.waxpby --numberFaulty 1 --faulty 0 " +  str(seed) + " > ./test_" + str(i) + "/test_" +str(i) + ".txt"
#    os.system("aprun -n 1 ./test_waxpby --stateFile /u/sciteam/jccalho2/research/FlipIt/.foo --numberFaulty 1 --faulty 0 " +  str(seed) + " > ./test_" + str(i) + "/test_" +str(i) + ".txt")
    #os.system("mpirun -np 1 ./test_waxpby --stateFile /home/aperson/research/FlipIt/.waxpby --numberFaulty 1 --faulty 0 " +  str(seed) + " > ./test_" + str(i) + "/test_" +str(i) + ".txt")
#    seed += 1
#    os.system("mv log.*.txt ./test_" + str(i) + "/")

