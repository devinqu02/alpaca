#include <stdio.h>
#include <stdlib.h>

#include "alpaca_tasks.h"

TASK(task_1);
TASK(task_2);
TASK(task_3);

NV_GLOBAL int count = 0;
NV_GLOBAL int count_priv;

TASK(task_3) {
    count_priv = count;
    for (int i = 0; i < 10000000; i++) {
        count_priv++;
    }
    printf("Task 3 Count: %d\n", count);
    pre_commit(&count, &count_priv, sizeof(count));
    TRANSITION_TO(task_1);
}

TASK(task_2) {
    count_priv = count;
    for (int i = 0; i < 10000000; i++) {
        count_priv++;
    }
    printf("Task 2 Count: %d\n", count);
    pre_commit(&count, &count_priv, sizeof(count));
    TRANSITION_TO(task_3);
}

TASK(task_1) {
    count_priv = count;
    for (int i = 0; i < 10000000; i++) {
        count_priv++;
    }
    printf("Task 1 Count: %d\n", count);
    pre_commit(&count, &count_priv, sizeof(count));
    TRANSITION_TO(task_2);
}

//NO WAR
MAIN_TASK {
    count = 69;
    printf("Program execution started. Entry Count: %d\n", count);
    TRANSITION_TO(task_1);
}

INITIALIZATION {
    printf("Just powered on :). Setting up sensors. Count on init is %d\n",
           count);
}