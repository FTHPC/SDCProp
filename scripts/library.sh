#!/bin/bash

#####################################################################
#
# This file is licensed under the University of Illinois/NCSA Open 
# Source License. See LICENSE.TXT for details.
#
#####################################################################

#####################################################################
#
# Name: library.sh
#
# Description: Generates the Instrumentationion library that the program is 
#		linked against. This library holds the user accessable 
#		functions of SDCProp as well as the internal functions 
#		that flip bits.
#
#####################################################################

if [[ -a $SDCPROP_PATH/lib/libInstrumentation.so ]]
 	then
	rm $SDCPROP_PATH/lib/libInstrumentation.so
fi

cd $SDCPROP_PATH/src/instrumentation/
make clean
make


if [[ -a libinstrumentation.so ]]; then
	cp libinstrumentation.so $SDCPROP_PATH/lib/
	# copy the library to a location that is in the library path
	if [ "$(whoami)" != "root" ]; then
		echo "Warning: Unable to copy propgation tracking library to '/usr/local/lib'"
		echo ""
		echo "You can link to the propagation tracking library using:"
	else
		cp libinstrumentation.so /usr/local/lib
		echo "You can link to the instrumentation library using:"
        echo "    -L$(SDCPROP_PATH)/lib/ -l:libinstrumentation.so"
	fi
        rm libinstrumentation.so

	echo "    -L$SDCPROP_PATH/lib -linstrumentation"
else
	echo "Error: Unable to make instrumentation library!"
fi

cd $SDCPROP_PATH/scripts/

