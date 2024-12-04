#include <stdio.h>
#include <stdlib.h>

#include "alpaca_tasks.h"

TASK(task_1);
TASK(task_2);
TASK(task_3);

NV_GLOBAL int count = 0;

TASK(task_3) {
    for (int i = 0; i < 10000000; i++) {
        count++;
    }
    printf("Task 3 Count: %d\n", count);
    TRANSITION_TO(task_1);
}

TASK(task_2) {
    for (int i = 0; i < 10000000; i++) {
        count++;
    }
    printf("Task 2 Count: %d\n", count);
    TRANSITION_TO(task_3);
}

TASK(task_1) {
    for (int i = 0; i < 10000000; i++) {
        count++;
    }
    printf("Task 1 Count: %d\n", count);
    if (count > 100000000) {
        TRANSITION_TO(end);
    }
    TRANSITION_TO(task_2);
}

MAIN_TASK {
    count = 69;
    printf("Program execution started. Entry Count: %d\n", count);
    TRANSITION_TO(task_1);
}

INITIALIZATION {
    printf("Just powered on :). Setting up sensors. Count on init is %d\n",
           count);
}