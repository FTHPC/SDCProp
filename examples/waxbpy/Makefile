CXX=$(SDCPROP_PATH)/scripts/sdcprop-cc
CC=$(SDCPROP_PATH)/scripts/sdcprop-cc
#LINKER=$(SDCPROP_PATH)/scripts/sdcprop-cc
CXX=mpicxx
LINKER=mpicxx
CPP_OPT_FLAGS = -std=c99 -O0 -g 
SYS_LIB =-lm


TARGET = test_waxpby

CXXFLAGS= $(CPP_OPT_FLAGS) $(OMP_FLAGS) $(USE_OMP) $(USE_MPI) $(MPI_INC) -I$(SDCPROP_PATH)/include -I$(FLIPIT_PATH)/include -I/home/jcalhoun/research/mpich-install/include

LIB_PATHS= $(SYS_LIB) -L$(FLIPIT_PATH)/lib -lcorrupt -ldl -L/usr/lib/ -lm -L$(SDCPROP_PATH)/lib/ -l:libinstrumentation.so -lstdc++ $(LFLAGS)
#LIB_PATHS= $(SYS_LIB) -L$(FLIPIT_PATH)/lib -lcorrupt -ldl -L/usr/lib/ -L$(SDCPROP_PATH)/lib -l:libinstrumentation.so -lstdc++ $(LFLAGS)



$(TARGET): waxpby.o main.o
	$(LINKER) $(CXXFLAGS) waxpby.o main.o $(LIB_PATHS) -o $(TARGET)

waxpby.o: waxpby.c
	$(SDCPROP_PATH)/scripts/sdcprop-cc $(CXXFLAGS) -c waxpby.c $(LIB_PATHS) -o waxpby.o

main.o: main.c
	$(SDCPROP_PATH)/scripts/sdcprop-cc $(CXXFLAGS) -c main.c $(LIB_PATHS) -o main.o

test:
	@echo "Not implemented yet..."

clean:
	@rm -f *.o  *.bc *~ $(TARGET) $(TARGET).exe test_waxpby BBID.txt *.bc *.LLVM.txt *.LLVM.bin $(FLIPIT_PATH)/.waxpby $(FLIPIT_PATH)/.foo 
