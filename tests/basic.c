#include "alpaca_tasks.h"

#include <stdio.h>

NV_GLOBAL int a, b, c;

MAIN_TASK {
    ++b;
    c = a;
    a = c + 1;
    printf("%d %d %d\n", a, b, c);

    TRANSITION_TO(end);
}

INITIALIZATION {}
