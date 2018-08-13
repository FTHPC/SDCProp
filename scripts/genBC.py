#!/usr/bin/python
#####################################################################
#
# This file is licensed under the University of Illinois/NCSA Open 
# Source License. See LICENSE.TXT for details.
#
#####################################################################

#####################################################################
#
# Name: genBC.py
#
# Description: Generates a bitcode header file that allows multiple 
#		source files to be compiled with SDCProp at the same 
#		time.
#
#####################################################################

import subprocess
import string
from config import *
'''import os

SDCPROP_PATH = os.environ['SDCPROP_PATH']
FLIPIT_PATH = os.environ['FLIPIT_PATH']
'''
# compile Instrumentation.cpp to LLVM bitcode
os.system("clang -emit-llvm -c -I/"+ FLIPIT_PATH + "/include/ " + MPI_FLAGS + " -o tmp.bc " + SDCPROP_PATH +"/src/instrumentation/Instrumentation.cpp")
#BW#os.system("clang -emit-llvm -c -I/"+ FLIPIT_PATH + "/include/ -I/usr/include/mpich -I//opt/cray/mpt/7.3.0/gni/mpich-gnu/4.9/include -o tmp.bc " + SDCPROP_PATH +"/src/instrumentation/Instrumentation.cpp")

cmd = ["llvm-link", "-d", "tmp.bc"]
p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
out, err = p.communicate()

# extract the function signitures and other info needed for the header file
# since they are defined in the IR we need to replace define with declare to make a header file
outfile = open(SDCPROP_PATH + "/src/instrumentation/Instrumentation.ll", "w")
attributes = []
for l in err.split("\n"):
	
	if "define" in l and "__" in l and not "linkonce_odr" in l:
		s = string.replace(l, "define", "declare", 1)
		s = string.replace(s, "internal", "", 1)
		outfile.write(s[0:-2])
		outfile.write("\n")
		a = s[0:-2].split(" ")[-1]
		if a not in attributes:
			attributes.append(a)
	elif "%struct._IO" == l[0:11]:
		outfile.write(l +"\n")
	elif "target datalayout = " in l or "target triple = " in l:
		outfile.write(l + "\n")
        elif "argBuf" in l:
            print l
        else:
		for i in attributes:
			if "attributes " +i in l:
				outfile.write(l +"\n")
outfile.close()

os.system("clang -emit-llvm -c -o " + SDCPROP_PATH + "/src/instrumentation/Instrumentation.bc " + SDCPROP_PATH+"/src/instrumentation/Instrumentation.ll")
os.system("rm tmp.bc")
