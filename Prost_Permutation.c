#include <stdio.h>
#include <linux/types.h>
#include <assert.h>

#include "Log.h"

#define IND(x, y) (x + y * 4)

#define D 32  // count of bits
#define T 18 // rounds
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
                set_bit(_s_state[IND(x,y)], z, res[4 * x + y]);
            }
        }
    }

}

static inline state_cell rotleft(state_cell value, int shift) {
    return (value << shift) | (value >> (sizeof(value) * 8 - shift));
}

static inline state_cell rotright(state_cell value, int shift) {
    return (value >> shift) | (value << (sizeof(value) * 8 - shift));
}

static inline state_cell msb(__u32 value)
{
    assert(value > 0);

    state_cell r = 0;
    while (value >>= 1) {
        r++;
    }
    return r;
}

static inline void add_constants(int round)
{
    assert(round >= 0 && round < T);

    state_cell c1 = msb(0x75817b9d);
    state_cell c2 = msb(0xb2c5fef0);
    c1 = 1 << c1;
    c2 = 1 << c2;

    int x, y, j;
    for (x = 0; x < 4; ++x) {
        for (y = 0; y < 4; ++y) {
            j = 4 *x + y;
            if (j % 2 == 0) {
                _s_state[IND(x,y)] ^= (rotleft(c1, round + j));
            } else {
                _s_state[IND(x,y)] ^= (rotleft(c2, round + j));
            }
        }
    }
}


static inline void shift_planes(int round)
{
    assert(round >= 0 && round < T);

    int P[2][4] = {{0, 4, 12, 26}, {1, 24, 26, 31}};

    int x, y;
    for (x = 0; x < 4; ++x) {
        for (y = 0; y < 4; ++y) {
            _s_state[IND(x, y)] = rotright(_s_state[IND(x, y)], P[round%2][x]);
        }
    }
}

static inline void shift_planes_inv(int round)
{
    assert(round >= 0 && round < T);

    int P[2][4] = {{0, 4, 12, 26}, {1, 24, 26, 31}};

    int x, y;
    for (x = 0; x < 4; ++x) {
        for (y = 0; y < 4; ++y) {
            _s_state[IND(x, y)] = rotleft(_s_state[IND(x, y)], P[round%2][x]);
        }
    }
}


void prost_permutation(__u8 *input, size_t size_in
                       , __u8 *output, size_t size_out)
{
    map_byte_stream_to_state(input, size_in);
    int round = 0;
    for (round = 0; round < T; ++round) {
        sub_rows();
        mix_slices();
        shift_planes(round);
        add_constants(round);
    }
    map_state_to_byte_stream(output, size_out);
}

void prost_permutation_inverse(__u8 *input, size_t size_in
                               , __u8 *output, size_t size_out)
{
    map_byte_stream_to_state(input, size_in);
    int round;
    for (round = T - 1; round >= 0; --round) {
        add_constants(round);
        shift_planes_inv(round);
        mix_slices();
        sub_rows();
    }
    map_state_to_byte_stream(output, size_out);
}

#undef set_bit
#undef get_bit
