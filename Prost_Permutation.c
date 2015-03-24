#include <stdio.h>
#include <linux/types.h>
#include <assert.h>

#include "Log.h"

#define IND(x, y) (x + y * 4)

#define D 32  // count of bits
typedef __u32 state_cell;
static state_cell _s_state[16];

#ifdef DEBUG

#define get_bit(cell, pos) \
            get_bit_real(cell, pos)

#define set_bit(cell, pos, value) \
            set_bit_real(&cell, (pos), (value))


static int get_bit_real(state_cell cell, int pos)
{
    assert(pos >= 0 && pos < 32);
    return (cell & (1<<pos)) ? 1 : 0;
}

static void set_bit_real(state_cell *cell, int pos, int value)
{
    assert(pos >= 0 && pos < 32);
    if (value) {
        *cell = *cell | (1<<pos);
    }
    else {
        *cell = *cell & (~(1<<pos));
    }
}
#else
#define get_bit(cell, pos)  (cell & (1<<pos))
#define set_bit(cell, pos)  (cell | (1<<pos))
#endif


static inline void map_byte_stream_to_state(__u8 *bs, size_t size)
{
    // d = 32    l = 4

    assert(size == 64); // 	# 4 * 4 * d / 8

    int x = 0, y = 0;

    for(x = 0; x < 4; ++x) {
        for(y = 0; y < 4; ++y) {
            _s_state[IND(x,y)] = (bs[16*x + 4*y] << (8 * 3)) +
                            (bs[16*x + 4*y + 1] << (8 * 2)) +
                            (bs[16*x  + 4*y + 2] << (8 * 1)) +
                             bs[16*x + 4*y + 3];
        }
    }
}

static inline void map_state_to_byte_stream(__u8 *bs, size_t size)
{
    assert(size == 64); // 	# 4 * 4 * d / 8
    int x = 0, y = 0, tmp = 0;

    for(x = 0; x < 4; ++x) {
        for(y = 0; y < 4; ++y) {
            tmp = _s_state[IND(x,y)];
            bs[16*x + 4*y] = (tmp >> (8 * 3)) & 0xFF;
            bs[16*x + 4*y + 1] = (tmp >> (8 * 2)) & 0xFF;
            bs[16*x  + 4*y + 2] = (tmp >> (8 * 1)) & 0xFF;
            bs[16*x + 4*y + 3] = tmp & 0xFF;
        }
    }
}

static inline void print_state(char *msg)
{
    int x = 0, y = 0;
    printf("=================state  %s =================>>>\n", msg);
    for(x = 0; x < 4; ++x) {
        for(y = 0; y < 4; ++y) {
            printf("[%d:%d] (%08x) ", x, y, _s_state[IND(x,y)]);
        }
        printf("\n");
    }
    printf("=================state=================<<<\n");
}

static inline void sub_rows(void)
{
    __u8 S[] = { 0x00, 0x04, 0x08, 0x0f, 0x01, 0x05, 0x0e, 0x09,
                 0x02, 0x07, 0x0a, 0x0c, 0x0b, 0x0d, 0x06, 0x03};
    state_cell tmp = 0;
    int y = 0, z = 0;
    for (y = 0; y < 4; ++y) {
        for (z = 0; z < D; ++z) {
            tmp = 0;
            set_bit(tmp, 3, get_bit(_s_state[IND(3,y)], z));
            set_bit(tmp, 2, get_bit(_s_state[IND(2,y)], z));
            set_bit(tmp, 1, get_bit(_s_state[IND(1,y)], z));
            set_bit(tmp, 0, get_bit(_s_state[IND(0,y)], z));
            tmp = S[tmp];
            set_bit(_s_state[IND(3,y)], z, get_bit(tmp, 3));
            set_bit(_s_state[IND(2,y)], z, get_bit(tmp, 2));
            set_bit(_s_state[IND(1,y)], z, get_bit(tmp, 1));
            set_bit(_s_state[IND(0,y)], z, get_bit(tmp, 0));
        }
    }
}

static inline void mix_slices(void)
{
    __u8 M[16][16] = {
                        {1,0,0,0,1,0,0,1,0,0,1,0,1,0,1,1},
                        {0,1,0,0,1,0,0,0,0,0,0,1,1,0,0,1},
                        {0,0,1,0,0,1,0,0,1,1,0,0,1,0,0,0},
                        {0,0,0,1,0,0,1,0,0,1,1,0,0,1,0,0},
                        {1,0,0,1,1,0,0,0,1,0,1,1,0,0,1,0},
                        {1,0,0,0,0,1,0,0,1,0,0,1,0,0,0,1},
                        {0,1,0,0,0,0,1,0,1,0,0,0,1,1,0,0},
                        {0,0,1,0,0,0,0,1,0,1,0,0,0,1,1,0},
                        {0,0,1,0,1,0,1,1,1,0,0,0,1,0,0,1},
                        {0,0,0,1,1,0,0,1,0,1,0,0,1,0,0,0},
                        {1,1,0,0,1,0,0,0,0,0,1,0,0,1,0,0},
                        {0,1,1,0,0,1,0,0,0,0,0,1,0,0,1,0},
                        {1,0,1,1,0,0,1,0,1,0,0,1,1,0,0,0},
                        {1,0,0,1,0,0,0,1,1,0,0,0,0,1,0,0},
                        {1,0,0,0,1,1,0,0,0,1,0,0,0,0,1,0},
                        {0,1,0,0,0,1,1,0,0,0,1,0,0,0,0,1}
    };

    __u8 mul[16];
    __u8 res[16];
    int i = 0, j = 0, z = 0;
    int x = 0, y = 0;

    for (z = 0; z < D; ++z) {
        //init
        for (x = 0; x < 4; ++x) {
            for (y = 0; y < 4; ++y) {
                mul[4 * x + y] = get_bit(_s_state[IND(x,y)], z);
            }
        }
        //multiply matrix
        for (i = 0; i < 16; ++i) {
            res[i] = 0;
            for (j = 0; j < 16; ++j) {
                res[i] ^= M[i][j] * mul[j];
            }
        }
        //set bits
        for (x = 0; x < 4; ++x) {
            for (y = 0; y < 4; ++y) {
                set_bit(_s_state[IND(x,y)], z, mul[4 * x + y]);
            }
        }
    }

}



void check_mapping(__u8 *input, size_t size, __u8 *output)
{
    map_byte_stream_to_state(input, size);
    //sub_rows();
    //print_state("after sub row");
    mix_slices();
    print_state("after mix state");
    //sub_rows();
    mix_slices();
    map_state_to_byte_stream(output, size);
}

#undef set_bit
#undef get_bit
