#####################################################################
#
# This file is licensed under the University of Illinois/NCSA Open 
# Source License. See LICENSE.TXT for details.
#
#####################################################################

#####################################################################
#
# Name: config.py
#
# Description: Default configuration file that is read by flipit-cc
#		flipit-cc will look in the cwd when compiling for this 
#		file, and if not found it will use the one in the 
#		SDCProp repo.
#
#####################################################################

############ LLVM BUILD TYPE ################
BUILD_TYPE = "Release+Asserts"
#BUILD_TYPE = "Debug+Asserts"

############### Injector Parameters ##################
#
#    config - config file used by the compiler pass
#    funcList - list of functions that are faulty
#    prob - probability that instuction is faulty
#    byte - which byte is faulty (0-7) -1 random
#    singleInj - one injection per active rank (0 or 1)
#    ptr - add code to inject into pointers (0 or 1)
#    arith - add code to inject into mathematics (0 or 1)
#    ctrl - add code to inject into control (0 or 1)
#
#####################################################
config = "WAXPBY.config"
funcList = "\"\""
prob = 0#1e-9
byte = -1
singleInj = 1
ptr = 1
arith = 1
ctrl = 1
stateFile = "waxpby"
############# Library Parameters #####################
#
#    FLIPIT_PATH - Path to FlipIt repo
#    LLVM_BULID_PATH - Path to LLVM build directory
#    SHOW - libraries and path wraped by mpicc 
#
#####################################################

import os
SDCPROP_PATH = os.environ['SDCPROP_PATH'] 
FLIPIT_PATH = os.environ['FLIPIT_PATH'] 
LLVM_BUILD_PATH = os.environ['LLVM_BUILD_PATH'] 
SHOW = "  -I/usr/local/include -L/usr/local/lib -I/home/jcalhoun/reserach/mpich-install/include -L/home/jcalhoun/research/mpich-install/lib -lmpich -lopa -lmpl -lrt -lpthread "

########### Files to NOT inject inside ###############
notInject = [" ", " "] #["CoMD.c", "parallel.c", "yamlOutput.c", "ljForce.c", "cmdLineParser.c"]

############ Default Compiler ########################
cc = "mpicc"

############ Verbose compiler output ################
verbose = True 


