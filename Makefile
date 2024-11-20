CXXFLAGS = -rdynamic $(shell llvm-config --cxxflags) -g -O0 -fPIC
TEST_DIR = tests

# # Replace % with test name to compile test
# test-licm-lpt-%: test-lpt-% licm.so dead_code_elimination.so
# 	opt -enable-new-pm=0 -load ./licm.so -loop-invariant-code-motion $*-landing_pad.bc -o $*-licm.bc
# 	llvm-dis $*-licm.bc 
# 	opt -enable-new-pm=0 -load ./dead_code_elimination.so -dead-code-eliminate $*-licm.bc -o $*-final.bc
# 	llvm-dis $*-final.bc 

# # Replace % with test name to compile test
# test-licm-%: licm.so dead_code_elimination.so %-m2r.bc
# 	opt -enable-new-pm=0 -load ./licm.so -loop-invariant-code-motion $*-m2r.bc -o $*-licm.bc
# 	llvm-dis $*-licm.bc 
# 	opt -enable-new-pm=0 -load ./dead_code_elimination.so -dead-code-eliminate $*-licm.bc -o $*-final.bc
# 	llvm-dis $*-final.bc 

# test-lpt-%: landing_pad.so %-m2r.bc
# 	opt -enable-new-pm=0 -load ./landing_pad.so -landing-pad $*-m2r.bc -o $*-landing_pad.bc
# 	llvm-dis $*-landing_pad.bc 

# run-%: %.bc dyn_inst_cnt.so
# 	opt -enable-new-pm=0 -load ./dyn_inst_cnt.so -dyn-inst-cnt $*.bc -o temp.bc
# 	clang temp.bc -o out
# 	./out

# %.so: %.o
# 	$(CXX) -dylib -shared -o $@ $^

# # Pattern rule to generate bitcode files
# %-m2r.bc: $(TEST_DIR)/%.c
# 	clang -Xclang -disable-O0-optnone -O0 -emit-llvm -fno-discard-value-names -c $< -o $*.bc
# 	opt -mem2reg $*.bc -o $@
# 	llvm-dis $*-m2r.bc 

all: bin/emulator bin/instrumented_app

bin/emulator: src/emulator.c
	clang -Wall -Wextra src/emulator.c -o bin/emulator

bin/instrumented_app: tests/simple.c src/alpaca_runtime.c src/emulator_instrumentation.c
	clang -Wall -Wextra -Iinclude -o bin/instrumented_app -fno-pie -no-pie tests/simple.c src/alpaca_runtime.c src/emulator_instrumentation.c

app: tests/simple.c alpaca_runtime.c 
	clang -Wall -Wextra -Iinclude -o bin/app -fno-pie -no-pie tests/simple.c src/alpaca_runtime.c 

clean:
	rm -f *.o *~ *.so *.bc *.ll *.out bin/*

.PHONY: clean all
