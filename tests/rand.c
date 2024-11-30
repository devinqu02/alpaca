#include "alpaca_tasks.h"

#include <stdio.h>
#include <stdlib.h>

NV_GLOBAL int iter, limit;
NV_GLOBAL double sum;

TASK(loop) {
    if (iter == limit) {
        printf("average: %f\n", sum / limit);
        TRANSITION_TO(end);
    }

    sum += rand();
    ++iter;

    TRANSITION_TO(loop);
}

MAIN_TASK {
    TRANSITION_TO(loop);
}

INITIALIZATION {
    limit = 100000;
}

