#include "alpaca_tasks.h"

#include <stdio.h>

NV_GLOBAL int a, b, c, d;

MAIN_TASK {
    ++b;
    c = a;
    a = c + 1;
    printf("%d %d %d %d\n", a, b, c, d);

    TRANSITION_TO(end);
}

INITIALIZATION {}
