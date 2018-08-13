# Makefile for SDC propagation pass


CXX=$(LLVM_BUILD_PATH)/bin/clang
LLVM_CONFIG=$(LLVM_BUILD_PATH)/bin/llvm-config

CXXFLAGS= -Wall -O0 -g -std=c++11 -fno-rtti -I$(SDCPROP_PATH)/include/ -DFLIPIT -I$(FLIPIT_PATH)/include
CXXFLAGS += $(shell $(LLVM_CONFIG) --cxxflags) -fPIC 

GCC_MAJOR=$(shell gcc -v  2>&1 /dev/null | grep -i "gcc version" | awk -F" " '{split($$3, a, ".")} END{print a[1]}')

ifneq ($(GCC_MAJOR),4)
	CXXFLAGS += -DUSE_OLD_ABI=1
endif

LDFLAGS = $(shell $(LLVM_CONFIG) --ldflags) -L $(FLIPIT_PATH)/lib -lFlipIt

ifeq ($(shell uname),Darwin)
SHARED_LIB_OPT  = -bundle -undefined dynamic_lookup
else
SHARED_LIB_OPT  = -shared -Wl,
endif

all: libSDCProp.so

libSDCProp.so:SDCProp.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $(SHARED_LIB_OPT) $<

clean:
	rm -rf *.o *.so
