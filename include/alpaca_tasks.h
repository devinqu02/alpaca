#ifndef _ALPACA_TASKS_H
#define _ALPACA_TASKS_H

#include <stdbool.h>
#include <stdint.h>

#define TRANSITION_TO(func_name)   \
    transition_to_arg = func_name; \
    JUMP_TO(transition_to)
#define JUMP_TO(func_name) __attribute__((musttail)) return func_name()
#define TASK(func_name) \
    __attribute__((annotate("alpaca_task"))) void func_name(void)
#define MAIN_TASK TASK(main_task)
#define INITIALIZATION void initialization(void)
#define NV_GLOBAL __attribute__((section("nv_data")))

MAIN_TASK;       // This is the first task to run (like main())
INITIALIZATION;  // This will run everytime device powers on
TASK(end);       // TRANSITION_TO(end) to finish your program (like exit(0))

extern void (*transition_to_arg)(void);
void transition_to(void);
void pre_commit(void* orig, void* priv_buff, unsigned size);

void handle_load(void*, void*, uint16_t*, int, unsigned);
void handle_store(void*, void*, uint16_t*, int, unsigned);

void sync_priv(void*, void*, unsigned, unsigned);
void pre_commit_array(void*, void*, unsigned, unsigned);

#ifdef STATS
#define PRINTF(fmt, ...)           \
    do { /* No operation needed */ \
    } while (0)
#else
#define PRINTF(fmt, ...) printf(fmt, ##__VA_ARGS__)
#endif

typedef struct program_pos {
    void (*next_task)(
        void);       // This is the task that will be rerun on power failure
    bool to_commit;  // Whether program is in commiting phase from last task
} program_pos_t;

#endif  // _ALPACA_TASKS_H
