
#include <stdarg.h>
#include <stdio.h>

#include "alpaca_tasks.h"

#define GLOBAL_SB(type, name, ...) GLOBAL_SB_(type, name, ##__VA_ARGS__, 3, 2)
#define GLOBAL_SB_(type, name, size, n, ...) GLOBAL_SB##n(type, name, size)
#define GLOBAL_SB2(type, name, ...) NV_GLOBAL type _global_##name
#define GLOBAL_SB3(type, name, size) NV_GLOBAL type _global_##name[size]

#define GV(type, ...) GV_(type, ##__VA_ARGS__, 2, 1)
#define GV_(type, i, n, ...) GV##n(type, i)
#define GV1(type, ...) _global_##type
#define GV2(type, i) _global_##type[i]

#define ENTRY_TASK(task_name) \
    MAIN_TASK { TRANSITION_TO(task_name); }

#define TASK_REF(task_name) task_name

#ifdef STATS
#define LOG(format, ...)                   \
    do { /* No operation needed for LOG */ \
    } while (0)

#define BLOCK_PRINTF_BEGIN()                 \
    do { /* No operation needed for BEGIN */ \
    } while (0)
#define BLOCK_PRINTF(fmt, ...)     \
    do { /* No operation needed */ \
    } while (0)
#define BLOCK_PRINTF_END()                 \
    do { /* No operation needed for END */ \
    } while (0)
#else
#define LOG(format, ...) \
    fprintf(stderr, "[%s:%d] " format "\n", __FILE__, __LINE__, ##__VA_ARGS__)

#define BLOCK_PRINTF_BEGIN()                 \
    do { /* No operation needed for BEGIN */ \
    } while (0)
#define BLOCK_PRINTF(fmt, ...) printf(fmt, ##__VA_ARGS__)
#define BLOCK_PRINTF_END()                 \
    do { /* No operation needed for END */ \
    } while (0)
#endif

typedef void(task_t)(void);

typedef struct {
    uint8_t x;
    uint8_t y;
    uint8_t z;
} threeAxis_t_8;

/* Square root by Newton's method */
uint16_t sqrt16(uint32_t x) {
    uint16_t hi = 0xffff;
    uint16_t lo = 0;
    uint16_t mid = ((uint32_t)hi + (uint32_t)lo) >> 1;
    uint32_t s = 0;

    while (s != x && hi - lo > 1) {
        mid = ((uint32_t)hi + (uint32_t)lo) >> 1;
        s = mid * mid;
        if (s < x)
            lo = mid;
        else
            hi = mid;
    }

    return mid;
}