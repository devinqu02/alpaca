CC = clang
CFLAGS = -Wall -Wextra -Iinclude

CXX = clang++  # Use clang++ for C++ files

SRC_DIR = src
LLVM_DIR = src/llvm
BIN_DIR = bin
TEST_DIR = tests


all: $(BIN_DIR)/emulator $(BIN_DIR)/simple.out

# Compile LLVM pass
$(BIN_DIR)/alpaca_pass.so: $(LLVM_DIR)/alpaca_pass.cpp $(LLVM_DIR)/find_war.cpp $(LLVM_DIR)/privatize.cpp
	$(CXX) -Iinclude -shared -fPIC -rdynamic $(shell llvm-config --cxxflags) -g -O0 -o $@ $^

# Compile Emulator
$(BIN_DIR)/emulator: $(SRC_DIR)/emulator.c
	$(CC) $(CFLAGS) -o $@ $^

# Link tests with runtime and instrumentation
$(BIN_DIR)/%.out: $(BIN_DIR)/%.o $(SRC_DIR)/alpaca_runtime.o  $(SRC_DIR)/emulator_instrumentation.o
	$(CC) $(CFLAGS) -o $@ -fno-pie -no-pie $^

# Compile tests
$(BIN_DIR)/%.o: $(TEST_DIR)/%.c $(BIN_DIR)/alpaca_pass.so
	$(CC) $(CFLAGS) -O0 -Xclang -disable-O0-optnone -emit-llvm -S $< -o $(BIN_DIR)/$*.ll
	opt -mem2reg -enable-new-pm=0 -load $(BIN_DIR)/alpaca_pass.so -alpaca-pass -simplifycfg $(BIN_DIR)/$*.ll | llc -filetype=obj -o $@
# TODO add more passes / switch to new pass manager

clean:
	rm -f $(BIN_DIR)/*

.PHONY: clean all
