#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "alpaca_tasks.h"

// Task to transition to at the end of the program
#ifdef STATS
NV_GLOBAL volatile unsigned load_count = 0, store_count = 0;
NV_GLOBAL volatile unsigned load_size = 0, store_size = 0;
extern char __start_nv_data[];
extern char __stop_nv_data[];
TASK(end) {
    printf("%d loads, %d stores\n", load_count, store_count);
    printf("%u bytes loaded, %u bytes stored\n", load_size, store_size);
    printf("%ld bytes used\n", __stop_nv_data - __start_nv_data);
    return;
}

void track_load(void* ptr, unsigned size) {
    if (__start_nv_data <= (char*)ptr && (char*)ptr <= __stop_nv_data) {
        ++load_count;
        load_size += size;
    }
}

void track_store(void* ptr, unsigned size) {
    if (__start_nv_data <= (char*)ptr && (char*)ptr <= __stop_nv_data) {
        ++store_count;
        store_size += size;
    }
}
#else
TASK(end) { return; }
#endif

// Pass in an argument to transition_to without changing function signature of
// transition_to
void (*transition_to_arg)(void);
// Transition to next task
void transition_to(void) { JUMP_TO(transition_to_arg); }

// Main function
int main() {
    // Call initialization function
    initialization();
    main_task();
    return 0;
}
