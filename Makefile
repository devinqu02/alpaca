CC = clang
CFLAGS = -Wall -Wextra -Iinclude

CXX = clang++  # Use clang++ for C++ files

SRC_DIR = src
LLVM_DIR = $(SRC_DIR)/llvm
BIN_DIR = bin
TEST_DIR = tests
BENCHMARKS_DIR = $(TEST_DIR)/benchmarks

LLVM_CXXFLAGS = $(shell llvm-config --cxxflags)

# Source files
BENCHMARK_FILES := $(wildcard $(BENCHMARKS_DIR)/*.c)
BENCHMARK_BINARIES := $(patsubst $(BENCHMARKS_DIR)/%.c, $(BIN_DIR)/benchmarks_%.out, $(BENCHMARK_FILES))

all: $(BIN_DIR)/emulator $(BIN_DIR)/simple.out

# Benchmarks: Generate binaries for all benchmarks
benchmarks: $(BENCHMARK_BINARIES)
	@echo "Benchmarks compiled successfully."

# Compile LLVM pass
$(BIN_DIR)/alpaca_pass.so: $(LLVM_DIR)/alpaca_pass.cpp $(LLVM_DIR)/find_war.cpp $(LLVM_DIR)/privatize.cpp $(LLVM_DIR)/pass_helper.cpp
	$(CXX) -Iinclude -shared -fPIC -rdynamic $(LLVM_CXXFLAGS) -g -O0 -o $@ $^

# Compile Emulator
$(BIN_DIR)/emulator: $(SRC_DIR)/emulator.c
	$(CC) $(CFLAGS) -o $@ $^

# Link tests with runtime and instrumentation
$(BIN_DIR)/%.out: $(BIN_DIR)/%.o $(SRC_DIR)/alpaca_runtime.o $(SRC_DIR)/emulator_instrumentation.o
	$(CC) $(CFLAGS) -o $@ -fno-pie -no-pie $^

# Compile benchmarks (TODO add load/store count pass)
$(BIN_DIR)/benchmarks_%.bc: $(BENCHMARKS_DIR)/%.c
	$(CC) $(CFLAGS) -O0 -Xclang -disable-O0-optnone -fno-discard-value-names -emit-llvm -S $< -o $(BIN_DIR)/benchmarks_$*.bc

# Compile tests
$(BIN_DIR)/%.bc: $(TEST_DIR)/%.c
	$(CC) $(CFLAGS) -O0 -Xclang -disable-O0-optnone -fno-discard-value-names -emit-llvm -S $< -o $(BIN_DIR)/$*.bc

# Run passes and finish compiling tests
$(BIN_DIR)/%.o: $(BIN_DIR)/%.bc $(BIN_DIR)/alpaca_pass.so
	opt -enable-new-pm=0 -mem2reg $(BIN_DIR)/$*.bc -o $(BIN_DIR)/$*.bc
	llvm-dis $(BIN_DIR)/$*.bc
	opt -enable-new-pm=0 -load $(BIN_DIR)/alpaca_pass.so -alpaca-pass -simplifycfg $(BIN_DIR)/$*.bc | llc -filetype=obj -o $@
# (TODO add passes to run after)

# Clean target
clean:
	rm -f $(BIN_DIR)/*

.PHONY: clean all benchmarks
