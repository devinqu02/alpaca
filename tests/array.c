#include "alpaca_tasks.h"

#include <stdio.h>

NV_GLOBAL int a[2], b[2];

MAIN_TASK {
    PRINTF("%d\n", a[0]);
    ++a[0];
    PRINTF("%d\n", a[0]);

    b[1] = a[1];

    PRINTF("%d\n", a[1]);
    ++a[1];
    PRINTF("%d\n", a[1]);

    TRANSITION_TO(end);
}

INITIALIZATION {}
