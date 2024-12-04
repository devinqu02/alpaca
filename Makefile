CC = clang
CFLAGS = -Wall -Wextra -Iinclude
CPPFLAGS = -DSTATS
#remove -dstats when not collecting stats

CXX = clang++  # Use clang++ for C++ files

SRC_DIR = src
LLVM_DIR = $(SRC_DIR)/llvm
BIN_DIR = bin
TEST_DIR = tests
BENCHMARKS_DIR = benchmarks

LLVM_CXXFLAGS = $(shell llvm-config --cxxflags)

RUNTIME_PASSES = -enable-new-pm=0 -load $(BIN_DIR)/stats_pass.so -mem2reg -stats-pass -simplifycfg
TEST_PASSES = -enable-new-pm=0 -load $(BIN_DIR)/alpaca_pass.so -load $(BIN_DIR)/stats_pass.so -mem2reg -alpaca-pass -stats-pass -simplifycfg

# Source files
BENCHMARK_FILES := $(wildcard $(BENCHMARKS_DIR)/*.c)
BENCHMARK_SUFFIXES = no_alpaca privatize_all war_only alpaca
# Generate targets for all versions of each benchmark
BENCHMARK_TARGETS := $(foreach suffix, $(BENCHMARK_SUFFIXES), $(addsuffix _$(suffix).out, $(patsubst $(BENCHMARKS_DIR)/%.c, $(BIN_DIR)/benchmarks_%, $(BENCHMARK_FILES))))

all: $(BIN_DIR)/emulator $(BIN_DIR)/simple.out


# Compile LLVM passes
$(BIN_DIR)/alpaca_pass.so: $(LLVM_DIR)/alpaca_pass.cpp $(LLVM_DIR)/find_war.cpp $(LLVM_DIR)/privatize.cpp $(LLVM_DIR)/pass_helper.cpp
	$(CXX) $(CPPFLAGS) -Iinclude -shared -fPIC -rdynamic $(LLVM_CXXFLAGS) -g -O0 -o $@ $^

$(BIN_DIR)/stats_pass.so: $(LLVM_DIR)/stats_pass.cpp $(LLVM_DIR)/pass_helper.cpp
	$(CXX) $(CPPFLAGS) -Iinclude -shared -fPIC -rdynamic $(LLVM_CXXFLAGS) -g -O0 -o $@ $^

# Compile Emulator
$(BIN_DIR)/emulator: $(SRC_DIR)/emulator.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $^

# Compile Runtime (.o)
$(BIN_DIR)/%_runtime.o: $(SRC_DIR)/%_runtime.c $(BIN_DIR)/stats_pass.so
	$(CC) $(CFLAGS) $(CPPFLAGS) -O0 -Xclang -disable-O0-optnone -fno-discard-value-names -emit-llvm -S $< -o $(BIN_DIR)/$*_runtime.bc
	opt $(RUNTIME_PASSES) $(BIN_DIR)/$*_runtime.bc | llc -filetype=obj -o $@

# Compile tests
$(BIN_DIR)/%.bc: $(TEST_DIR)/%.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -O0 -Xclang -disable-O0-optnone -fno-discard-value-names -emit-llvm -S $< -o $@

# Run passes and finish compiling tests
$(BIN_DIR)/%.o: $(BIN_DIR)/%.bc $(BIN_DIR)/alpaca_pass.so $(BIN_DIR)/stats_pass.so
	opt $(TEST_PASSES) -enable-alpaca -use-war-analysis -use-vbm-privatization $< | llc -filetype=obj -o $@

# Link tests with runtime and emulation instrumentation
$(BIN_DIR)/%.out: $(BIN_DIR)/%.o $(BIN_DIR)/alpaca_runtime.o $(SRC_DIR)/emulator_instrumentation.o
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ -fno-pie -no-pie $^



# Benchmarks: Generate binaries for all benchmarks
benchmarks: $(BENCHMARK_TARGETS)
	@echo "Benchmarks compiled successfully."


# Link benchmarks with runtime but not emulation instrumentation
$(BIN_DIR)/benchmarks_%.out: $(BIN_DIR)/benchmarks_%.o $(BIN_DIR)/alpaca_runtime.o
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ -fno-pie -no-pie $^

# 1: No Alpaca (link with empty runtime, enable_alpaca=true, use_war_analysis=false, use_vbm_privatization=false)
$(BIN_DIR)/benchmarks_%_no_alpaca.bc: $(BENCHMARKS_DIR)/%.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -O0 -Xclang -disable-O0-optnone -fno-discard-value-names -emit-llvm -S $< -o $@

$(BIN_DIR)/benchmarks_%_no_alpaca.o: $(BIN_DIR)/benchmarks_%_no_alpaca.bc $(BIN_DIR)/alpaca_pass.so $(BIN_DIR)/stats_pass.so
	opt $(TEST_PASSES) $< | llc -filetype=obj -o $@

$(BIN_DIR)/benchmarks_%_no_alpaca.out: $(BIN_DIR)/benchmarks_%_no_alpaca.o $(BIN_DIR)/empty_runtime.o
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ -fno-pie -no-pie $^

# 2: Privatize All (enable_alpaca=true, use_war_analysis=false, use_vbm_privatization=false)
$(BIN_DIR)/benchmarks_%_privatize_all.bc: $(BENCHMARKS_DIR)/%.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -O0 -Xclang -disable-O0-optnone -fno-discard-value-names -emit-llvm -S $< -o $@

$(BIN_DIR)/benchmarks_%_privatize_all.o: $(BIN_DIR)/benchmarks_%_privatize_all.bc $(BIN_DIR)/alpaca_pass.so $(BIN_DIR)/stats_pass.so
	opt $(TEST_PASSES) -enable-alpaca $< | llc -filetype=obj -o $@

# 3: WAR Only (enable_alpaca=true, use_war_analysis=true, use_vbm_privatization=false)
$(BIN_DIR)/benchmarks_%_war_only.bc: $(BENCHMARKS_DIR)/%.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -O0 -Xclang -disable-O0-optnone -fno-discard-value-names -emit-llvm -S $< -o $@

$(BIN_DIR)/benchmarks_%_war_only.o: $(BIN_DIR)/benchmarks_%_war_only.bc $(BIN_DIR)/alpaca_pass.so $(BIN_DIR)/stats_pass.so
	opt $(TEST_PASSES) -enable-alpaca -use-war-analysis $< | llc -filetype=obj -o $@

# 4: Full Alpaca (enable_alpaca=true, use_war_analysis=false, use_vbm_privatization=true)
$(BIN_DIR)/benchmarks_%_alpaca.bc: $(BENCHMARKS_DIR)/%.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -O0 -Xclang -disable-O0-optnone -fno-discard-value-names -emit-llvm -S $< -o $@

$(BIN_DIR)/benchmarks_%_alpaca.o: $(BIN_DIR)/benchmarks_%_alpaca.bc $(BIN_DIR)/alpaca_pass.so $(BIN_DIR)/stats_pass.so
	opt $(TEST_PASSES) -enable-alpaca -use-war-analysis -use-vbm-privatization $< | llc -filetype=obj -o $@

# Clean target
clean:
	rm -f $(BIN_DIR)/*

.PHONY: clean all benchmarks
