#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "alpaca_tasks.h"

// Task to transition to at the end of the program
#ifdef STATS
NV_GLOBAL volatile int load_count = 0, store_count = 0;
extern char __start_nv_data[];
extern char __stop_nv_data[];
TASK(end) {
    printf("%d loads, %d stores\n", load_count, store_count);
    printf("%ld bytes used\n", __stop_nv_data - __start_nv_data);
    return;
}
#else
TASK(end) { return; }
#endif

// Program position to track the next task and commit status. Double buffered to
// atomically update curr_program_pos
NV_GLOBAL program_pos_t program_pos_0 = {.next_task = main_task,
                                         .to_commit = false};
NV_GLOBAL program_pos_t program_pos_1;
NV_GLOBAL program_pos_t* curr_program_pos = &program_pos_0;

// Commit list of (orig, priv_buff, size)
NV_GLOBAL unsigned list_size = 0;
// extern void* commit_list_orig[]; // Create in LLVM (with size equal to
// max_commit_list) extern void* commit_list_priv_buff[]; extern unsigned
// commit_list_size[];
NV_GLOBAL void* commit_list_orig[20];  // TODO remove
NV_GLOBAL void* commit_list_priv_buff[20];
NV_GLOBAL unsigned commit_list_size[20];

NV_GLOBAL volatile uint16_t curr_version = 0;

// Add to commit list
void pre_commit(void* orig, void* priv_buff, unsigned size) {
    commit_list_orig[list_size] = orig;
    commit_list_priv_buff[list_size] = priv_buff;
    commit_list_size[list_size] = size;
    list_size++;
}

void commit_all(void) {
    while (list_size > 0) {
        int ind = list_size - 1;
        memcpy(commit_list_orig[ind], commit_list_priv_buff[ind],
               commit_list_size[ind]);
        list_size = ind;  // This statement is the atomic commit
    }
}

void handle_load(void* orig, void* priv_buff, uint16_t* vbm, int i,
                 unsigned size) {
    if (vbm[i] != curr_version) {
        memcpy((char*)priv_buff + i * size, (char*)orig + i * size, size);
    }
}

void handle_store(void* orig, void* priv_buff, uint16_t* vbm, int i,
                  unsigned size) {
    if (vbm[i] != curr_version) {
        vbm[i] = curr_version;
        pre_commit((char*)orig + i + size, (char*)priv_buff + i * size, size);
    }
}

// Pass in an argument to transition_to without changing function signature of
// transition_to
void (*transition_to_arg)(void);
// Transition to next task
void transition_to(void) {
    // Update curr_program_pos to {.next_task = next_task, .to_commit = true}
    // [Checkpoint]
    // Atomically update program_pos using double buffering
    program_pos_t* next_program_pos =
        (curr_program_pos == &program_pos_0 ? &program_pos_1 : &program_pos_0);
    next_program_pos->next_task = transition_to_arg;
    next_program_pos->to_commit = true;
    curr_program_pos = next_program_pos;

    commit_all();

    // Update curr_program_pos to {.next_task = next_task, .to_commit = false}
    // [Checkpoint]
    curr_program_pos->to_commit = false;

    ++curr_version;

    JUMP_TO(curr_program_pos->next_task);
}

// Main function
int main() {
    // Call initialization function
    initialization();

    ++curr_version;

    if (curr_program_pos->to_commit) {
        commit_all();
    } else {
        list_size = 0;
    }
    curr_program_pos->next_task();

    return 0;
}
