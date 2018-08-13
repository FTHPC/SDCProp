#!/bin/bash

#####################################################################
#
# This file is licensed under the University of Illinois/NCSA Open 
# Source License. See LICENSE.TXT for details.
#
#####################################################################

#####################################################################
#
# Name: createPass.sh
#
# Description: Compiles the LLVM compiler pass of SDCProp
#
#####################################################################

make clean

#echo "
#Making Compiler Pass Library:"
#make -f Makefile-Lib
#rm -rf Release+Asserts
#rm -rf Debug+Asserts

echo "
Making Compiler Pass:"
make -f Makefile

