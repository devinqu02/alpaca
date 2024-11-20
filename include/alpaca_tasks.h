
#ifndef _ALPACA_TASKS_H
#define _ALPACA_TASKS_H

#include <stdbool.h>

#define TRANSITION_TO(func_name)   \
    transition_to_arg = func_name; \
    JUMP_TO(transition_to)
#define JUMP_TO(func_name) __attribute__((musttail)) return func_name()
#define TASK(func_name) void func_name(void)
#define MAIN_TASK TASK(main_task)
#define INITIALIZATION void initialization(void)
#define NV_GLOBAL __attribute__((section("nv_data")))

MAIN_TASK;       // This is the first task to run (like main())
INITIALIZATION;  // This will run everytime device powers on
TASK(end);       // TRANSITION_TO(end) to finish your program (like exit(0))

extern void (*transition_to_arg)(void);
void transition_to(void);
void pre_commit(void* orig, void* priv_buff, unsigned size);

typedef struct program_pos {
    void (*next_task)(
        void);       // This is the task that will be rerun on power failure
    bool to_commit;  // Whether privatized buffer will be  will be done before
                     // next_task is called
} program_pos_t;

extern NV_GLOBAL program_pos_t *curr_program_pos;

#endif  // _ALPACA_TASKS_H
