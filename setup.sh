#!/bin/bash

#####################################################################
#
# This file is licensed under the University of Illinois/NCSA Open 
# Source License. See LICENSE.TXT for details.
#
#####################################################################

#####################################################################
#
# Name: setup.sh
#
# Description: Sets up and and reconfigures existing LLVM build, 
#				$LLVM_PATH, with the fault injector FlitIt, 
#               $SDCPROP_PATH.
#
#####################################################################
# setup.sh



# Build instrumentation library
echo "

Building the instrumentation library..."
cd $SDCPROP_PATH
mkdir $SDCPROP_PATH/lib
cd $SDCPROP_PATH/scripts/
./library.sh
echo "

Making bitcode header."
./genBC.py

# Include directory for easier compilation
mkdir $SDCPROP_PATH/include/
ln -s -f $SDCPROP_PATH/src/instrumentation/Instrumentation.h $SDCPROP_PATH/include/
ln -s -f $SDCPROP_PATH/src/instrumentation/Instrumentation.bc $SDCPROP_PATH/include/
ln -s -f $SDCPROP_PATH/src/instrumentation/Types.h $SDCPROP_PATH/include/
echo "Done!"


echo "

Creating pass..."
cd $SDCPROP_PATH/scripts/
./findLLVMHeaders.py $SDCPROP_PATH/src/pass/SDCProp.h
cd $SDCPROP_PATH/src/pass
make clean -f Makefile
make -f Makefile
cp *.so $SDCPROP_PATH/lib/
echo "Done!"



echo "

Build Finished."

