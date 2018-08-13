MPI_FLAGS= -I/home/jcalhoun/research/mpich-install/include -L/home/jcalhoun/research/mpich-install/lib -Wl,-rpath -Wl,/home/jcalhoun/research/mpich-install/lib -Wl,--enable-new-dtags -lmpi -lrt -lpthread
CFLAGS= -c -g -fPIC -I$(FLIPIT_PATH)/include $(MPI_FLAGS)
CXXFLAGS= $(CFLAGS) -std=c++11 
CXX=g++
MPICC=mpicc
#CXX=cc
#CXX=g++
CC=clang
TEST_PATH =./tests/
INST_DIR=/home/jcalhoun/research/build/lib/Transforms/SDCProp

#all: MemoryAccess.o  Memory.o Execution.o Instrumentation.o 
all: MemoryAccess.o Memory.o Execution.o Instrumentation.o #Alloc.o mpi_wrapper.o
	rm -f libinstrumentation.so
	$(MPICC)  -o libinstrumentation.so -shared MemoryAccess.o Memory.o Execution.o Instrumentation.o -Wl,--no-whole-archive -ldl -Wl,--no-whole-archive -lstdc++ -L$(FLIPIT_PATH)/lib -lcorrupt
	#gcc  -o libinstrumentation.so -shared MemoryAccess.o BasicBlock.o Memory.o Execution.o Instrumentation.o Alloc.o mpi_wrapper.o -Wl,--no-whole-archive -ldl -Wl,--no-whole-archive -lstdc++

shared: MemoryAccess.o Memory.o Execution.o Instrumentation.o
	rm -f libinstrumentation.a
	ar -cvq libinstrumentation.a MemoryAccess.o Memory.o Execution.o Instrumentation.o 
	cp libinstrumentation.a ../tests/

bitcode: Instrumentation.cpp
	clang -c -emit-llvm Instrumentation.cpp
	llvm-dis Instrumentation.bc 
	grep "define" Instrumentation.ll 
	clang -c -emit-llvm Instrumentation.ll


MemoryAccess.o: MemoryAccess.cpp
	$(CXX) $(CXXFLAGS) $<

#BasicBlock.o: BasicBlock.cpp
#	$(CXX) $(CXXFLAGS) $<

Memory.o: Memory.cpp
	$(CXX) $(CXXFLAGS) $<

Execution.o: Execution.cpp
	$(CXX) $(CXXFLAGS) $<

Instrumentation.o: Instrumentation.cpp
	$(CXX) $(CXXFLAGS) $<

#Alloc.o: Alloc.c
#	gcc -fPIC -g -c $<

#mpi_wrapper.o: mpi_wrapper.c
#	mpicc -fPIC -g -c $<

clean:
	rm -f MemoryAccess.o Memory.o Execution.o Instrumentation.o Alloc.o
	rm -f libinstrumentation.so
    
test: $(TEST_PATH)test1.out $(TEST_PATH)test2.out $(TEST_PATH)test3.out
	cd $(TEST_PATH)
	./test1.out
	./test2.out
	./test3.out

test-SpMV: $(TEST_PATH)/SpMV/SpMV.out
	cd $(TEST_PATH)/SpMV/
	pwd
	./SpMV.out
	cd -
SpMV.out: $(TEST_PATH)/SpMV/Makefile
	cd $(TEST_PATH)
	pwd
	cd ./SpMV/
	pwd
	make
	$(CC) -c -emit-llvm -o util.bc util.c
	llvm-link -o linked.bc ../Instrumentation.bc util.bc 
	opt -load $(LLVM_BUILD_PATH)/Debug+Asserts/lib/SDCProp.so -SDCProp linked.bc -o spmv.bc
	$(CC) -g -c -o SpMV.o spmv.bc
	$(CC) -o SpMV.out SpMV.o -L$(INST_DIR)/tests/ -linstrumentation -lstdc++
    
    
test1.out: $(TEST_PATH)test1.c
	cd $(TEST_PATH)
	pwd
	$(CC) -c -emit-llvm -o test1.bc $<
	llvm-link -o linked1.bc Instrumentation.bc test1.bc 
	opt -load $(LLVM_BUILD_PATH)/Debug+Asserts/lib/SDCProp.so -SDCProp linked1.bc -o test1_mod.bc
	$(CC) -g -c -o test1.o test1_mod.bc
	$(CC) -o test1.out test1.o -L$(INST_DIR)/tests/ -linstrumentation -lstdc++
	cd -

test2.out: $(TEST_PATH)test2.c
	cd $(TEST_PATH)
	$(CC) -c -emit-llvm -o test2.bc $<
	llvm-link -o linked2.bc Instrumentation.bc test2.bc 
	opt -load $(LLVM_BUILD_PATH)/Debug+Asserts/lib/SDCProp.so -SDCProp linked2.bc -o test2_mod.bc
	$(CC) -g -c -o test2.o test2_mod.bc
	$(CC) -o test2.out test2.o -L$(INST_DIR)/tests/ -linstrumentation -lstdc++
	cd -

test3.out: $(TEST_PATH)test3.c
	cd $(TEST_PATH)
	$(CC) -c -emit-llvm -o test3.bc $<
	llvm-link -o linked3.bc Instrumentation.bc test3.bc 
	opt -load $(LLVM_BUILD_PATH)/Debug+Asserts/lib/SDCProp.so -SDCProp linked3.bc -o test3_mod.bc
	$(CC) -g -c -o test3.o test3_mod.bc
	$(CC) -o test3.out test3.o -L$(INST_DIR)/tests/ -linstrumentation -lstdc++
	cd -
