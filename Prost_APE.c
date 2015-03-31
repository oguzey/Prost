#include <stdio.h>
#include <linux/types.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "Prost_Permutation.h"
#include "Prost_APE.h"

#define r (256 / 8)
#define c r

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

static Data* prost_strip_padding(__u8 *msg, size_t size);

/**********************************************************/
/*
 * Helpful functions
 */
Data *data_create(__u8 *a_data, size_t a_size)
{
    Data *data = malloc(sizeof(Data));
    data->data = a_data;
    data->size = a_size;
    return data;
}

void data_destroy(Data *data)
{
    if (data) {
        free(data->data);
        free(data);
    }
}

void data_print(Data *data, char *msg)
{
    printf("%s\n", msg);
    if (data) {
        int i = 0;
        for (; i < (int)data->size; ++i) {
            printf("%02x ", data->data[i]);
        }
        printf("\n");
    } else {
        printf("(null)\n");
    }
}

/**********************************************************/

#define print_bit_uint(bit, value) do {                  \
    printf("%s = ", #value);                        \
    long int tmp = 0;                               \
    int i = 0;                                      \
    int size = bit;                \
    for (i = size - 1;i >= 0; --i) {                          \
        tmp = 1;                                   \
        tmp <<= i;                                 \
        printf("%s", (tmp & value) ? "1" : "0");   \
    }                                               \
    printf("\n");                                   \
    } while(0)


static void generate_key_from_password(const char *pass, __u8 *key)
{
    memset(key, 0, r);

    /* x[t] = x[t - 3] ^ x[t - 5] ^ x[t - 9] ^ x[t - 20]*/

    __u32 LFSR = 0;
    __u8 bit_res = 0;
    __u32 cell_0, cell_11, cell_15, cell_17;
	int len = strlen(pass);
    int i;
    //TODO: need to use cryptographic hash function
    for (i = 0; i < len; ++i) {
        int j = i % 4;
        LFSR ^= pass[i] << j * 8;
    }

    /* set init value */
    LFSR = LFSR % 0x0fffff;
    cell_0 = 1 << 0;
    cell_11 = 1 << 11;
    cell_15 = 1 << 15;
    cell_17 = 1 << 17;
    for (i = 0; i < r * 8; ++i) {

        bit_res = LFSR & 1;
        key[i/8] ^= (bit_res << (i % 8));

        bit_res = 0;
        if (LFSR & cell_0) bit_res ^= 1;
        if (LFSR & cell_11) bit_res ^= 1;
        if (LFSR & cell_15) bit_res ^= 1;
        if (LFSR & cell_17) bit_res ^= 1;

        LFSR >>= 1;
        LFSR ^= bit_res << 19;
    }
}


static Data* create_X(Data *nonce, Data *associated_data)
{
    size_t N_A_size = 0;
    if (nonce) {
        N_A_size += nonce->size;
    }
    if (associated_data) {
        N_A_size += associated_data->size;
    }
    size_t x_size = N_A_size / r;
    if (N_A_size % r != 0) {
        x_size = (N_A_size + r - (N_A_size % r)) / r;
    }
    printf("x_size = %d   N_A_size= %d\n", (int)x_size, (int)N_A_size);

    __u8 *ret = malloc(x_size * r);
    memset(ret, 0, x_size * r);
    if (nonce)
        memcpy(ret + x_size * r - nonce->size, nonce->data, nonce->size);

    if (associated_data)
        memcpy(ret + x_size * r - N_A_size, associated_data->data
                                            , associated_data->size);
    ret[x_size * r - N_A_size - 1] = 0x80;
    return data_create(ret, x_size * r);
}

void prost_encrypt(Data *data, Data *nonce, Data *assoc_data, const char *pass,
                   Data **CT, Data ** tag)
{
    int i = 0, j = 0;

    __u8 V_new[ r * 2];
    memset(V_new, 0, r*2);

    __u8 V[ r * 2];
    generate_key_from_password(pass, V);
    memset(V + r, 0, r);
    //memset(V, 0x99, r);  // set key
    size_t pad_size = (data->size % r) == 0 ? 0 : r - (data->size % r);

    size_t all_size = data->size + pad_size;
    printf("Need to add %d bytes as pad\n", (int)pad_size);

    /* Pad M to positive multiple of r bits */

    size_t m_blocks = all_size / r;

    __u8 *M = malloc(all_size);
    memset(M, 0, all_size);
    for (i = 0; i < (int)data->size; ++i) {
        M[all_size - 1 - i] = data->data[data->size - i - 1];
    }
    M[all_size - 1 - i] = 0x80;


    __u8 *C = malloc(all_size);
    __u8 *T = malloc(r);

    /* Prepend N to A and pad to positive multiple of r bits */
    Data *X = create_X(nonce, assoc_data);
    size_t x_blocks = X->size / r;
    /* Process nonce and AD */

    for(i = x_blocks - 1; i >= 0; --i) {
        for (j = r - 1; j >= 0; --j) {
            V[r + j] ^= X->data[i*r + j];
        }

        prost_permutation(V, 2 * r, V_new, 2 * r);
        memcpy(V, V_new, 2 * r);
    }

    V[0] ^= 1;

    /* Process message */

    for(i = m_blocks - 1; i >= 0; --i) {
        for (j = r - 1; j >= 0; --j) {
            V[r + j] ^= M[i*r + j];
        }

        prost_permutation(V, 2 * r, V_new, 2 * r);
        memcpy(V, V_new, 2 * r);

        for (j = r - 1; j >= 0; --j) {
            C[i*r + j] = V[r + j];
        }
    }
    free(M);
    /* Compute tag */
    print_v(V, r, 1, "before tag" );

    for (j = r - 1; j >= 0; --j) {
        T[j] = V[j] ^ 0x99;
    }
    print_v(T, r, 1, "tag" );

    *CT = data_create(C, all_size);
    *tag = data_create(T, r);
}

int prost_decrypt(Data *CT, Data *tag, Data *nonce, Data *assoc_data,
                  const char *pass, Data **OT)
{
    int i, j;

    __u8 V_new[ r * 2];
    memset(V_new, 0, r*2);

    __u8 IV[ r * 2];
    generate_key_from_password(pass, IV);
    memset(IV + r, 0, r);
    //memset(IV, 0x99, r);  // set key

    /* Prepend N to A and pad to positive multiple of r bits */

    Data *X = create_X(nonce, assoc_data);
    size_t x_blocks = X->size / r;

    /* Process nonce and AD */

    for(i = x_blocks - 1; i >= 0; --i) {
        for (j = r - 1; j >= 0; --j) {
            IV[r + j] ^= X->data[i*r + j];
        }

        prost_permutation(IV, 2 * r, V_new, 2 * r);
        memcpy(IV, V_new, 2 * r);
    }
    IV[0] ^= 1;
    data_destroy(X);

    /* Set dummy C[0] = IV r for a smoother loop */
    assert(CT->size % r == 0);
    size_t ct_blocks = CT->size / r;
    __u8 *C = malloc((ct_blocks + 1) * r);
    memcpy(C, CT->data, CT->size  + r);

    for (j = r - 1; j >= 0; --j) {
        C[ct_blocks * r + j] = IV[r + j];
    }

    __u8 V[ r * 2];
    memset(V, 0, r*2);

    assert(tag->size == r);

    for (j = r - 1; j >= 0; --j) {
        V[j] = tag->data[j] ^ 0x99;
    }
    print_v(tag->data, r, 1, "dec tag" );
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
    free(C);
    print_v(M, ct_blocks*r, 1, "decr M");

    int is_fail = 0;
    for (i = r - 1; i >= 0; --i) {
        if (IV[i] != V[i]) {
            is_fail = 1;
            break;
        }
    }

    print_v(IV, r, 1, "decr IV fin");
    print_v(V, r, 1, "decr V fin");

    printf("CORRECT IS  %d\n", !is_fail);

    *OT = prost_strip_padding(M, CT->size);
    return is_fail;
}

static Data* prost_strip_padding(__u8 *msg, size_t size)
{
    size_t pad_size = 0;
    size_t i;
    for(i = 0; i < size-1; ++i) {
        if (msg[i] == 0) {
            pad_size++;
            if (msg[i+1] == 0x80) {
                pad_size++;
                break;
            }
        }
    }
    printf("Size pad is %d. Cut it!\n", (int)pad_size);
    size_t new_size = size - pad_size;
    __u8 *new_data = malloc(new_size);
    for (i = 0; i < new_size; ++i) {
        new_data[i] = msg[pad_size +i];
    }
    return data_create(new_data, new_size);
}
