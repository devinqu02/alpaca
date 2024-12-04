#include "alpaca_tasks.h"

#include <stdio.h>

NV_GLOBAL int a, b;

void f() {
    PRINTF("%d %d\n", a, b);
}

TASK(task2) {
    ++b;
    f();
    TRANSITION_TO(end);
}

TASK(task1) {
    ++a;
    f();
    TRANSITION_TO(task2);
}

MAIN_TASK {
    JUMP_TO(task1);
}

INITIALIZATION {}
