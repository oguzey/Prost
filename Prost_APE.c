#include <stdio.h>
#include <linux/types.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "Prost_Permutation.h"

#define r (256 / 8)
#define c r

//typedef struct {
//    __u8 data[r];
//} block;

//static block** concatenation_data_to_block(__u8 *one_data, size_t one_size,
//                            __u8 *two_data, size_t two_size)
//{
//    size_t all_size = one_size + two_size;

//    assert(all_size % r == 0);
//    assert(sizeof(block) - r == 0);

//    size_t count_blocks = all_size / r;

//    printf("all size %u , count blocks %u\n", all_size, count_blocks);

//    int i;
//    block **blocks = malloc(sizeof(block*) * count_blocks);
//    for (i = 0; i < count_blocks; ++i) {
//        blocks[i] = malloc(sizeof(block));
//    }
//    __u8 *all = malloc(all_size);
//    memcpy(all, one_data, one_size);
//    if (two_size) {
//        memcpy(all + one_data, two_data, two_size);
//    }
//    for (i = 0; i < all_size; ++i) {
//        blocks[i/r]->data[i%r] = all[i];
//        printf("block %d, cell data %d, all %d\n", i/r, i%r, all[i]);
//    }
//    return blocks;
//}

//static block** normalize_data(__u8 *data, size_t size)
//{
//    __u8 *add = NULL;
//    size_t size_add = r - (size % r);
//    printf("normalize_data| need add %d bytes\n", size_add);
//    if (size_add) {
//        add = malloc(sizeof(__u8) * size_add);
//        memset(add, 0, size_add);
//        add[0] = 0x80;
//    }
//    return concatenation_data_to_block(data, size, add, size_add);
//}

#define print_v(vector, size, reverse, msg) \
do {                        \
    int i;      \
    __u8 *v = vector; \
printf("%s\n", msg);        \
if (reverse) {      \
   for (i = size - 1; i >= 0; --i){     \
       printf("%02x ", v[i]);        \
   }        \
} else {    \
    for (i = 0; i < (int)size; ++i){   \
        printf("%02x ", v[i]);   \
    }   \
}   \
printf("\n");   \
} while(0)


void prost_encrypt(__u8 *data, size_t size
                   , __u8 **ct, size_t *size_ct, __u8 **tag, size_t *size_tag)
{
    int i = 0, j = 0;

    __u8 V_new[ r * 2];
    memset(V_new, 0, r*2);

    __u8 V[ r * 2];
    memset(V + r, 0, r);
    memset(V, 0x99, r);  // set key

    size_t size_add = (size % r) == 0 ? 0 : r - (size % r);
    printf("need add %d bytes\n", (int)size_add);

    /* Pad M to positive multiple of r bits */

    size_t m_size = (size + size_add) / r;

    __u8 *M = malloc(size + size_add);
    memset(M, 0, size + size_add);
    for (i = 0; i < (int)size; ++i) {
        M[size + size_add - 1 - i] = data[i];
    }
    M[size + size_add - 1 - i] = 0x80;


    __u8 *C = malloc(size + size_add);
    __u8 *T = malloc(r);

    /* Prepend N to A and pad to positive multiple of r bits */

    size_t x_size = 1;
    __u8 *X = malloc(r);
    memset(X, 0, r);
    X[r - 1] = 0x80;

    /* Process nonce and AD */

    for(i = x_size - 1; i >= 0; --i) {
        for (j = r - 1; j >= 0; --j) {
            V[r + j] ^= X[i*r + j];
        }

        prost_permutation(V, 2 * r, V_new, 2 * r);
        memcpy(V, V_new, 2 * r);
    }

    V[0] ^= 1;

    /* Process message */

    for(i = m_size - 1; i >= 0; --i) {
        for (j = r - 1; j >= 0; --j) {
            V[r + j] ^= M[i*r + j];
        }

        prost_permutation(V, 2 * r, V_new, 2 * r);
        memcpy(V, V_new, 2 * r);

        for (j = r - 1; j >= 0; --j) {
            C[i*r + j] = V[r + j];
        }
    }

    /* Compute tag */
    print_v(V, r, 1, "before tag" );

    for (j = r - 1; j >= 0; --j) {
        T[j] = V[j] ^ 0x99;
    }
    print_v(T, r, 1, "tag" );

    *ct = C;
    *tag = T;
    *size_ct = size + size_add;
    *size_tag = r;
}

int prost_decrypt(__u8 *ct, size_t ct_size
                  , __u8 *tag, size_t tag_size
                  , __u8 **output, size_t *size_output)
{
    int i, j;

    __u8 V_new[ r * 2];
    memset(V_new, 0, r*2);

    __u8 IV[ r * 2];
    memset(IV + r, 0, r);
    memset(IV, 0x99, r);  // set key

    /* Prepend N to A and pad to positive multiple of r bits */

    size_t x_size = 1;
    __u8 *X = malloc(r);
    memset(X, 0, r);
    X[r - 1] = 0x80;

    /* Process nonce and AD */

    for(i = x_size - 1; i >= 0; --i) {
        for (j = r - 1; j >= 0; --j) {
            IV[r + j] ^= X[i*r + j];
        }

        prost_permutation(IV, 2 * r, V_new, 2 * r);
        memcpy(IV, V_new, 2 * r);
    }
    IV[0] ^= 1;

    /* Set dummy C[0] = IV r for a smoother loop */
    assert(ct_size % r == 0);
    size_t ct_blocks = ct_size / r;
    __u8 *C = malloc((ct_blocks + 1) * r);
    memcpy(C, ct, ct_size  + r);

    for (j = r - 1; j >= 0; --j) {
        C[ct_blocks * r + j] = IV[r + j];
    }

    __u8 V[ r * 2];
    memset(V, 0, r*2);

    assert(tag_size == r);

    for (j = r - 1; j >= 0; --j) {
        V[j] = tag[j] ^ 0x99;
    }
    print_v(tag, r, 1, "dec tag" );
    print_v(V, r, 1, "dec V" );

    /* Process ciphertext */
    __u8 TMP[ r *2];
    memset(TMP, 0, r*2);

    __u8 *M = malloc(ct_blocks * r);
    memset(M, 0, ct_blocks * r);

    for(i = 0; i < (int)ct_blocks; ++i) {
        for (j = r - 1; j >= 0; --j) {
            TMP[r + j] = C[i*r + j];
            TMP[j] = V[j];
        }
        prost_permutation_inverse(TMP, 2 * r, V, 2 * r);
        for (j = r - 1; j >= 0; --j) {
            M[i*r + j] = V[r + j] ^ C[(i+1)*r + j];
        }
    }

    print_v(M, ct_blocks*r, 1, "decr M");

    int is_correct = 1;
    for (i = r - 1; i >= 0; --i) {
        if (IV[i] != V[i]) {
            is_correct = 0;
            break;
        }
    }

    print_v(IV, r, 1, "decr IV fin");
    print_v(V, r, 1, "decr V fin");

    printf("CORRECT IS  %d\n", is_correct);

    *output = M;
    *size_output = ct_blocks * r;
    return is_correct;
}
