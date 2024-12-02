#include "alpaca_tasks.h"

#include <stdio.h>

NV_GLOBAL int a[2];

MAIN_TASK {
    printf("%d\n", a[0]);
    ++a[0];
    printf("%d\n", a[0]);

    printf("%d\n", a[1]);
    ++a[1];
    printf("%d\n", a[1]);
}

INITIALIZATION {}
