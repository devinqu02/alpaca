#include <stdlib.h>

#include "translation.h"

#define SEED 4L
#define ITER 100
#define CHAR_BIT 8

static const char bits[256] = {
    0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4, /* 0   - 15  */
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, /* 16  - 31  */
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, /* 32  - 47  */
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, /* 48  - 63  */
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, /* 64  - 79  */
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, /* 80  - 95  */
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, /* 96  - 111 */
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, /* 112 - 127 */
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, /* 128 - 143 */
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, /* 144 - 159 */
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, /* 160 - 175 */
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, /* 176 - 191 */
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, /* 192 - 207 */
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, /* 208 - 223 */
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, /* 224 - 239 */
    4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8  /* 240 - 255 */
};
GLOBAL_SB(unsigned, n_0);
GLOBAL_SB(unsigned, n_1);
GLOBAL_SB(unsigned, n_2);
GLOBAL_SB(unsigned, n_3);
GLOBAL_SB(unsigned, n_4);
GLOBAL_SB(unsigned, n_5);
GLOBAL_SB(unsigned, n_6);

GLOBAL_SB(unsigned, func);

GLOBAL_SB(uint32_t, seed);
GLOBAL_SB(unsigned, iter);

TASK(task_init);
TASK(task_select_func);
TASK(task_bit_count);
TASK(task_bitcount);
TASK(task_ntbl_bitcnt);
TASK(task_ntbl_bitcount);
TASK(task_BW_btbl_bitcount);
TASK(task_AR_btbl_bitcount);
TASK(task_bit_shifter);
TASK(task_end);

void task_init() {
    LOG("init\r\n");

    GV(func) = 0;
    GV(n_0) = 0;
    GV(n_1) = 0;
    GV(n_2) = 0;
    GV(n_3) = 0;
    GV(n_4) = 0;
    GV(n_5) = 0;
    GV(n_6) = 0;

    TRANSITION_TO(task_select_func);
}

void task_select_func() {
    LOG("select func\r\n");

    GV(seed) = (uint32_t)SEED;  // for test, seed is always the same
    GV(iter) = 0;
    LOG("func: %u\r\n", GV(func));
    if (GV(func) == 0) {
        GV(func)++;
        TRANSITION_TO(task_bit_count);
    } else if (GV(func) == 1) {
        GV(func)++;
        TRANSITION_TO(task_bitcount);
    } else if (GV(func) == 2) {
        GV(func)++;
        TRANSITION_TO(task_ntbl_bitcnt);
    } else if (GV(func) == 3) {
        GV(func)++;
        TRANSITION_TO(task_ntbl_bitcount);
    } else if (GV(func) == 4) {
        GV(func)++;
        TRANSITION_TO(task_BW_btbl_bitcount);
    } else if (GV(func) == 5) {
        GV(func)++;
        TRANSITION_TO(task_AR_btbl_bitcount);
    } else if (GV(func) == 6) {
        GV(func)++;
        TRANSITION_TO(task_bit_shifter);
    } else {
        TRANSITION_TO(task_end);
    }
}
void task_bit_count() {
    LOG("bit_count\r\n");
    uint32_t tmp_seed = GV(seed);
    GV(seed) = GV(seed) + 13;
    unsigned temp = 0;
    if (tmp_seed) do
            temp++;
        while (0 != (tmp_seed = tmp_seed & (tmp_seed - 1)));
    GV(n_0) += temp;
    GV(iter)++;

    if (GV(iter) < ITER) {
        TRANSITION_TO(task_bit_count);
    } else {
        TRANSITION_TO(task_select_func);
    }
}
void task_bitcount() {
    LOG("bitcount\r\n");
    uint32_t tmp_seed = GV(seed);
    GV(seed) = GV(seed) + 13;
    tmp_seed = ((tmp_seed & 0xAAAAAAAAL) >> 1) + (tmp_seed & 0x55555555L);
    tmp_seed = ((tmp_seed & 0xCCCCCCCCL) >> 2) + (tmp_seed & 0x33333333L);
    tmp_seed = ((tmp_seed & 0xF0F0F0F0L) >> 4) + (tmp_seed & 0x0F0F0F0FL);
    tmp_seed = ((tmp_seed & 0xFF00FF00L) >> 8) + (tmp_seed & 0x00FF00FFL);
    tmp_seed = ((tmp_seed & 0xFFFF0000L) >> 16) + (tmp_seed & 0x0000FFFFL);
    GV(n_1) += (int)tmp_seed;
    GV(iter)++;

    if (GV(iter) < ITER) {
        TRANSITION_TO(task_bitcount);
    } else {
        TRANSITION_TO(task_select_func);
    }
}
int recursive_cnt(uint32_t x) {
    int cnt = bits[(int)(x & 0x0000000FL)];

    if (0L != (x >>= 4)) cnt += recursive_cnt(x);

    return cnt;
}
void task_ntbl_bitcnt() {
    LOG("ntbl_bitcnt\r\n");
    GV(n_2) += recursive_cnt(GV(seed));
    GV(seed) = GV(seed) + 13;
    GV(iter)++;

    if (GV(iter) < ITER) {
        TRANSITION_TO(task_ntbl_bitcnt);
    } else {
        TRANSITION_TO(task_select_func);
    }
}
void task_ntbl_bitcount() {
    LOG("ntbl_bitcount\r\n");
    GV(n_3) += bits[(int)(GV(seed) & 0x0000000FUL)] +
               bits[(int)((GV(seed) & 0x000000F0UL) >> 4)] +
               bits[(int)((GV(seed) & 0x00000F00UL) >> 8)] +
               bits[(int)((GV(seed) & 0x0000F000UL) >> 12)] +
               bits[(int)((GV(seed) & 0x000F0000UL) >> 16)] +
               bits[(int)((GV(seed) & 0x00F00000UL) >> 20)] +
               bits[(int)((GV(seed) & 0x0F000000UL) >> 24)] +
               bits[(int)((GV(seed) & 0xF0000000UL) >> 28)];
    GV(seed) = GV(seed) + 13;
    GV(iter)++;

    if (GV(iter) < ITER) {
        TRANSITION_TO(task_ntbl_bitcount);
    } else {
        TRANSITION_TO(task_select_func);
    }
}
void task_BW_btbl_bitcount() {
    LOG("BW_btbl_bitcount\r\n");
    union {
        unsigned char ch[4];
        long y;
    } U;

    U.y = GV(seed);

    GV(n_4) += bits[U.ch[0]] + bits[U.ch[1]] + bits[U.ch[3]] + bits[U.ch[2]];
    GV(seed) = GV(seed) + 13;
    GV(iter)++;

    if (GV(iter) < ITER) {
        TRANSITION_TO(task_BW_btbl_bitcount);
    } else {
        TRANSITION_TO(task_select_func);
    }
}
void task_AR_btbl_bitcount() {
    LOG("AR_btbl_bitcount\r\n");
    unsigned char *Ptr = (unsigned char *)&GV(seed);
    int Accu;

    Accu = bits[*Ptr++];
    Accu += bits[*Ptr++];
    Accu += bits[*Ptr++];
    Accu += bits[*Ptr];
    GV(n_5) += Accu;
    GV(seed) = GV(seed) + 13;
    GV(iter)++;

    if (GV(iter) < ITER) {
        TRANSITION_TO(task_AR_btbl_bitcount);
    } else {
        TRANSITION_TO(task_select_func);
    }
}
void task_bit_shifter() {
    LOG("bit_shifter\r\n");
    int i, nn;
    uint32_t tmp_seed = GV(seed);
    for (i = nn = 0; tmp_seed && (i < (sizeof(long) * CHAR_BIT));
         ++i, tmp_seed >>= 1)
        nn += (int)(tmp_seed & 1L);
    GV(n_6) += nn;
    GV(seed) = GV(seed) + 13;

    GV(iter)++;

    if (GV(iter) < ITER) {
        TRANSITION_TO(task_bit_shifter);
    } else {
        TRANSITION_TO(task_select_func);
    }
}

void task_end() {
    LOG("end\r\n");
    PRINTF("%u\r\n", GV(n_0));
    PRINTF("%u\r\n", GV(n_1));
    PRINTF("%u\r\n", GV(n_2));
    PRINTF("%u\r\n", GV(n_3));
    PRINTF("%u\r\n", GV(n_4));
    PRINTF("%u\r\n", GV(n_5));
    PRINTF("%u\r\n", GV(n_6));
    TRANSITION_TO(end);
    //	TRANSITION_TO(task_end);
}

ENTRY_TASK(task_init)
INITIALIZATION {}